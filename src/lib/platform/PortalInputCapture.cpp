/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "platform/PortalInputCapture.h"

#if HAVE_LIBPORTAL_INPUTCAPTURE

#include "base/Log.h"
#include "inputleap/protocol_types.h"
#include "platform/PortalInputCapture.h"

#include <sys/un.h> // for EIS fd hack, remove
#include <sys/socket.h> // for EIS fd hack, remove

namespace inputleap {

enum signals {
    SESSION_CLOSED,
    DISABLED,
    ACTIVATED,
    DEACTIVATED,
    ZONES_CHANGED,
    _N_SIGNALS,
};

PortalInputCapture::PortalInputCapture(EiScreen* screen, IEventQueue* events) :
    screen_(screen),
    events_(events),
    portal_(xdp_portal_new()),
    signals_(_N_SIGNALS)
{
    glib_main_loop_ = g_main_loop_new(nullptr, true);
    glib_thread_ = std::make_unique<Thread>([this](){ glib_thread(); });

    auto init_capture_cb = [](gpointer data) -> gboolean
    {
        return reinterpret_cast<PortalInputCapture*>(data)->init_input_capture_session();
    };

    g_idle_add(init_capture_cb, this);
}

PortalInputCapture::~PortalInputCapture()
{
    if (g_main_loop_is_running(glib_main_loop_))
        g_main_loop_quit(glib_main_loop_);

    if (glib_thread_) {
        glib_thread_->cancel();
        glib_thread_->wait();
        glib_thread_.reset();

        g_main_loop_unref(glib_main_loop_);
        glib_main_loop_ = nullptr;
    }

    if (session_) {
        XdpSession *parent_session = xdp_input_capture_session_get_session(session_);
        g_signal_handler_disconnect(G_OBJECT(parent_session), signals_[SESSION_CLOSED]);
        g_signal_handler_disconnect(session_, signals_[DISABLED]);
        g_signal_handler_disconnect(session_, signals_[ACTIVATED]);
        g_signal_handler_disconnect(session_, signals_[DEACTIVATED]);
        g_signal_handler_disconnect(session_, signals_[ZONES_CHANGED]);
        g_object_unref(session_);
    }

    for (auto b : barriers_) {
        g_object_unref(b);
    }
    barriers_.clear();
    g_object_unref(portal_);
}

gboolean PortalInputCapture::timeout_handler()
{
    return true; // keep re-triggering
}

int PortalInputCapture::fake_eis_fd()
{
    auto path = std::getenv("LIBEI_SOCKET");

    if (!path) {
        LOG_DEBUG("Cannot fake EIS socket, LIBEI_SOCKET environment variable is unset");
        return -1;
    }

    auto sock = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);

    // Dealing with the socket directly because nothing in lib/... supports
    // AF_UNIX and I'm too lazy to fix all this for a temporary hack
    int fd = sock;
    struct sockaddr_un addr = {
        .sun_family = AF_UNIX,
        .sun_path = {0},
    };
    std::snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

    auto result = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (result != 0) {
        LOG_DEBUG("Faked EIS fd failed: %s", strerror(errno));
    }

    return sock;
}

void PortalInputCapture::cb_session_closed(XdpSession* session)
{
    LOG_ERR("Our InputCapture session was closed, exiting.");
    g_main_loop_quit(glib_main_loop_);
    events_->add_event(EventType::QUIT);

    g_signal_handler_disconnect(session, signals_[SESSION_CLOSED]);
    signals_[SESSION_CLOSED] = 0;
}

void PortalInputCapture::cb_init_input_capture_session(GObject* object, GAsyncResult* res)
{
    LOG_DEBUG("Session ready");
    g_autoptr(GError) error = nullptr;

    auto session = xdp_portal_create_input_capture_session_finish(XDP_PORTAL(object), res, &error);
    if (!session) {
        LOG_ERR("Failed to initialize InputCapture session, quitting: %s", error->message);
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
        return;
    }

    session_ = session;

    auto fd = xdp_input_capture_session_connect_to_eis(session, &error);
    if (fd < 0) {
            LOG_ERR("Failed to connect to EIS: %s", error->message);

            // FIXME: Development hack to avoid having to assemble all parts just for
            // testing this code.
            fd = fake_eis_fd();

            if (fd < 0) {
                g_main_loop_quit(glib_main_loop_);
                events_->add_event(EventType::QUIT);
                return;
            }
    }
    // Socket ownership is transferred to the EiScreen
    events_->add_event(EventType::EI_SCREEN_CONNECTED_TO_EIS, screen_->get_event_target(),
                       create_event_data<int>(fd));

    // FIXME: the lambda trick doesn't work here for unknown reasons, we need
    // the static function
    signals_[DISABLED] = g_signal_connect(G_OBJECT(session), "disabled",
                                          G_CALLBACK(cb_disabled_cb),
                                          this);
    signals_[ACTIVATED] = g_signal_connect(G_OBJECT(session_), "activated",
                                            G_CALLBACK(cb_activated_cb),
                                            this);
    signals_[DEACTIVATED] = g_signal_connect(G_OBJECT(session_), "deactivated",
                                              G_CALLBACK(cb_deactivated_cb),
                                              this);
    signals_[ZONES_CHANGED] = g_signal_connect(G_OBJECT(session_), "zones-changed",
                                                G_CALLBACK(cb_zones_changed_cb),
                                                this);
    XdpSession *parent_session = xdp_input_capture_session_get_session(session);
    signals_[SESSION_CLOSED] = g_signal_connect(G_OBJECT(parent_session), "closed",
                                                G_CALLBACK(cb_session_closed_cb),
                                                this);

    cb_zones_changed(session_, nullptr);
}

void PortalInputCapture::cb_set_pointer_barriers(GObject* object, GAsyncResult* res)
{
    g_autoptr(GError) error = nullptr;

    auto failed_list = xdp_input_capture_session_set_pointer_barriers_finish(session_, res, &error);
    if (failed_list) {
        auto it = failed_list;
        while (it) {
            guint id;
            g_object_get(it->data, "id", &id, nullptr);

            for (auto elem = barriers_.begin(); elem != barriers_.end(); elem++) {
                if (*elem == it->data) {
                    int x1, x2, y1, y2;

                    g_object_get(G_OBJECT(*elem), "x1", &x1, "x2", &x2, "y1", &y1, "y2", &y2, nullptr);

                    LOG_WARN("Failed to apply barrier %d (%d/%d-%d/%d)", id, x1, y1, x2, y2);
                    g_object_unref(*elem);
                    barriers_.erase(elem);
                    break;
                }
            }
            it = it->next;
        }
    }
    g_list_free_full(failed_list, g_object_unref);

    enable();

}

gboolean PortalInputCapture::init_input_capture_session()
{
    LOG_DEBUG("Setting up the InputCapture session");
    xdp_portal_create_input_capture_session(
                portal_,
                nullptr, // parent
                static_cast<XdpInputCapability>(XDP_INPUT_CAPABILITY_KEYBOARD | XDP_INPUT_CAPABILITY_POINTER),
                nullptr, // cancellable
                [](GObject *obj, GAsyncResult *res, gpointer data) {
                reinterpret_cast<PortalInputCapture*>(data)->cb_init_input_capture_session(obj, res);
                },
                this);

    return false;
}

void PortalInputCapture::enable()
{
    if (!enabled_) {
        LOG_DEBUG("Enabling the InputCapture session");
        xdp_input_capture_session_enable(session_);
        enabled_ = true;
    }
}

void PortalInputCapture::disable()
{
    if (enabled_) {
        LOG_DEBUG("Disabling the InputCapture session");
        xdp_input_capture_session_disable(session_);
        enabled_ = false;
    }
}

void PortalInputCapture::release()
{
    LOG_DEBUG("Releasing InputCapture with activation id %d", activation_id_);
    xdp_input_capture_session_release(session_, activation_id_);
    is_active_ = false;
}

void PortalInputCapture::release(double x, double y)
{
    LOG_DEBUG("Releasing InputCapture with activation id %d at (%.1f,%.1f)", activation_id_, x, y);
    xdp_input_capture_session_release_at(session_, activation_id_, x, y);
    is_active_ = false;
}

void PortalInputCapture::cb_disabled(XdpInputCaptureSession* session, GVariant* options)
{
    LOG_DEBUG("PortalInputCapture::cb_disabled");

    if (!enabled_)
        return; // Nothing to do

    enabled_ = false;
    is_active_ = false;

    // FIXME: need some better heuristics here of when we want to enable again
    // But we don't know *why* we got disabled (and it's doubtfull we ever will), so
    // we just assume that the zones will change or something and we can re-enable again
    // ... very soon
    g_timeout_add(1000,
                  [](gpointer data) -> gboolean {
                      reinterpret_cast<PortalInputCapture*>(data)->enable();
                  return false;
                  },
                  this);
}

void PortalInputCapture::cb_activated(XdpInputCaptureSession* session, std::uint32_t activation_id,
                                      GVariant* options)
{
    LOG_DEBUG("PortalInputCapture::cb_activated() activation_id=%d", activation_id);

    gdouble x = 0, y = 0;
    if (options) {
        if (!g_variant_lookup(options, "cursor_position", "(dd)", &x, &y))
            LOG_WARN("Failed to get cursor_position");
    } else {
        LOG_WARN("Activation has no options!");
    }

    activation_id_ = activation_id;
    is_active_ = true;

    // Notify the server where the cursor is. The server calls leave() on the
    // primary when it switches to a secondary; leave() does NOT release the capture
    // (the cursor stays at the barrier). The capture is released in enter() when
    // returning to primary.
    screen_->notify_input_capture_activated((int32_t)x, (int32_t)y);

    // Safety watchdog: if the server did not switch to a secondary screen within
    // 1 second (e.g. no adjacent screen on this side), release the capture so
    // the cursor is never permanently frozen at a hot-corner.
    // We must NOT release when is_on_screen()=false — that means the server DID
    // switch and the capture is intentionally kept active during secondary control.
    g_timeout_add(1000, [](gpointer data) -> gboolean {
        auto* self = reinterpret_cast<PortalInputCapture*>(data);
        if (self->is_active_ && self->screen_->is_on_screen()) {
            LOG_WARN("InputCapture: safety-releasing stuck capture (no adjacent screen)");
            self->release();
        }
        return G_SOURCE_REMOVE;
    }, this);
}

void PortalInputCapture::cb_deactivated(XdpInputCaptureSession* session,
                                        std::uint32_t activation_id, GVariant* options)
{
    LOG_DEBUG("PortalInputCapture::cb_deactivated() activation id=%i", activation_id);
    is_active_ = false;
}

void PortalInputCapture::cb_zones_changed(XdpInputCaptureSession* session, GVariant* options)
{
    for (auto b : barriers_)
        g_object_unref(b);
    barriers_.clear();

    auto zones = xdp_input_capture_session_get_zones(session);
    while (zones != nullptr) {
        guint w, h;
        gint x, y;
        g_object_get(zones->data, "width", &w, "height", &h, "x", &x, "y", &y, nullptr  );

        LOG_DEBUG("Zone at %dx%d@%d,%d", w, h, x, y);

        int x1, x2, y1, y2;

        // Install barriers only where active_sides_ indicates a client screen is present.
        // This prevents capturing the cursor at edges with no adjacent screen (e.g. the
        // GNOME top panel) which would cause it to appear stuck.
        auto add_barrier = [&](const char* name, int bx1, int by1, int bx2, int by2) {
            auto id = barriers_.size() + 1;
            LOG_DEBUG("Barrier (%s) %zd at %d,%d-%d,%d", name, id, bx1, by1, bx2, by2);
            barriers_.push_back(XDP_INPUT_CAPTURE_POINTER_BARRIER(
                g_object_new(XDP_TYPE_INPUT_CAPTURE_POINTER_BARRIER,
                             "id", (guint)id,
                             "x1", bx1, "y1", by1,
                             "x2", bx2, "y2", by2,
                             nullptr)));
        };

        if (active_sides_ & kTopMask)
            add_barrier("top",    x,     y,     x+w-1, y);
        if (active_sides_ & kRightMask)
            add_barrier("right",  x+w,   y,     x+w,   y+h-1);
        if (active_sides_ & kLeftMask)
            add_barrier("left",   x,     y,     x,     y+h-1);
        if (active_sides_ & kBottomMask)
            add_barrier("bottom", x,     y+h,   x+w-1, y+h);

        zones = zones->next;
    }

    GList* list = nullptr;
    for (auto const &b : barriers_) {
        list = g_list_append(list, b);
    }

    xdp_input_capture_session_set_pointer_barriers(
                session_,
                list,
                nullptr, // cancellable
                [](GObject *obj, GAsyncResult *res, gpointer data)
                {
                    reinterpret_cast<PortalInputCapture*>(data)->cb_set_pointer_barriers(obj, res);
                },
                this);
}

void PortalInputCapture::set_active_sides(std::uint32_t sides)
{
    active_sides_ = sides;
    if (!session_)
        return;
    // Reschedule barrier setup in the glib thread since cb_zones_changed uses glib objects.
    auto* self = this;
    g_idle_add([](gpointer data) -> gboolean {
        auto* capture = reinterpret_cast<PortalInputCapture*>(data);
        if (capture->session_)
            capture->cb_zones_changed(capture->session_, nullptr);
        return G_SOURCE_REMOVE;
    }, self);
}

void PortalInputCapture::glib_thread()
{
    auto context = g_main_loop_get_context(glib_main_loop_);

    LOG_DEBUG("GLib thread running");

    while (g_main_loop_is_running(glib_main_loop_)) {
        Thread::testCancel();
        g_main_context_iteration(context, true);
    }

    LOG_DEBUG("Shutting down GLib thread");
}

} // namespace inputleap

#endif
