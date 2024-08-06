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

#include "base/Log.h"
#include "platform/PortalRemoteDesktop.h"

#include <sys/un.h> // for EIS fd hack, remove
#include <sys/socket.h> // for EIS fd hack, remove

namespace inputleap {

PortalRemoteDesktop::PortalRemoteDesktop(EiScreen *screen,
                                         IEventQueue* events) :
    screen_(screen),
    events_(events),
    portal_(xdp_portal_new())
{
    glib_main_loop_ = g_main_loop_new(nullptr, true);
    glib_thread_ = new Thread([this](){ glib_thread(); });

    reconnect(0);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
    if (g_main_loop_is_running(glib_main_loop_))
        g_main_loop_quit(glib_main_loop_);

    if (glib_thread_ != nullptr) {
        glib_thread_->cancel();
        glib_thread_->wait();
        delete glib_thread_;
        glib_thread_ = nullptr;

        g_main_loop_unref(glib_main_loop_);
        glib_main_loop_ = nullptr;
    }

    if (session_signal_id_)
        g_signal_handler_disconnect(session_, session_signal_id_);
    if (session_ != nullptr)
        g_object_unref(session_);
    g_object_unref(portal_);

    free(session_restore_token_);
}

gboolean PortalRemoteDesktop::timeout_handler()
{
    return true; // keep re-triggering
}

void PortalRemoteDesktop::reconnect(unsigned int timeout)
{
    auto init_cb = [](gpointer data) -> gboolean
    {
        return reinterpret_cast<PortalRemoteDesktop*>(data)->init_remote_desktop_session();
    };

    if (timeout > 0)
        g_timeout_add(timeout, init_cb, this);
    else
        g_idle_add(init_cb, this);
}

void PortalRemoteDesktop::cb_session_closed(XdpSession* session)
{
    LOG_ERR("Our RemoteDesktop session was closed, re-connecting.");
    g_signal_handler_disconnect(session, session_signal_id_);
    session_signal_id_ = 0;
    events_->add_event(EventType::EI_SESSION_CLOSED, screen_->get_event_target());

    // gcc warning "Suspicious usage of 'sizeof(A*)'" can be ignored
    g_clear_object(&session_);

    reconnect(1000);
}

void PortalRemoteDesktop::cb_session_started(GObject* object, GAsyncResult* res)
{
    g_autoptr(GError) error = nullptr;
    auto session = XDP_SESSION(object);
    auto success = xdp_session_start_finish(session, res, &error);
    if (!success) {
        LOG_ERR("Failed to start session");
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
        return;
    }

    session_restore_token_ = xdp_session_get_restore_token(session);

    // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
    // version in the impl.portal), i.e. you'll need an updated compositor on
    // top of everything...
    auto fd = -1;
#if HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS
    fd = xdp_session_connect_to_eis(session, &error);
#endif
    if (fd < 0) {
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
        return;
    }

    // Socket ownership is transferred to the EiScreen
    events_->add_event(EventType::EI_SCREEN_CONNECTED_TO_EIS, screen_->get_event_target(),
                        create_event_data<int>(fd));
}

void PortalRemoteDesktop::cb_init_remote_desktop_session(GObject* object, GAsyncResult* res)
{
    LOG_DEBUG("Session ready");
    g_autoptr(GError) error = nullptr;

    auto session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
    if (!session) {
        LOG_ERR("Failed to initialize RemoteDesktop session: %s", error->message);
        // This was the first attempt to connect to the RD portal - quit if that fails.
        if (session_iteration_ == 0) {
            g_main_loop_quit(glib_main_loop_);
            events_->add_event(EventType::QUIT);
        } else {
            this->reconnect(1000);
        }
        return;
    }

    session_ = session;
    ++session_iteration_;

    // FIXME: the lambda trick doesn't work here for unknown reasons, we need
    // the static function
    session_signal_id_ = g_signal_connect(G_OBJECT(session), "closed",
                                         G_CALLBACK(cb_session_closed_cb),
                                         this);

    LOG_DEBUG("Session ready, starting");
    xdp_session_start(session,
                      nullptr, // parent
                      nullptr, // cancellable
                      [](GObject *obj, GAsyncResult *res, gpointer data) {
                          reinterpret_cast<PortalRemoteDesktop*>(data)->cb_session_started(obj, res);
                      },
                      this);
}

#if !defined(HAVE_LIBPORTAL_CREATE_REMOTE_DESKTOP_SESSION_FULL)
static inline void
xdp_portal_create_remote_desktop_session_full(XdpPortal              *portal,
                                              XdpDeviceType           devices,
                                              XdpOutputType           outputs,
                                              XdpRemoteDesktopFlags   flags,
                                              XdpCursorMode           cursor_mode,
                                              XdpPersistMode          _unused1,
                                              const char             *_unused2,
                                              GCancellable           *cancellable,
                                              GAsyncReadyCallback     callback,
                                              gpointer                data)
{
    xdp_portal_create_remote_desktop_session(portal, devices, outputs,
                                             flags, cursor_mode, cancellable,
                                             callback, data);
}
#endif


gboolean PortalRemoteDesktop::init_remote_desktop_session()
{
    LOG_DEBUG("Setting up the RemoteDesktop session with restore token %s", session_restore_token_);
    xdp_portal_create_remote_desktop_session_full(
                portal_,
                static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD),
                XDP_OUTPUT_NONE,
                XDP_REMOTE_DESKTOP_FLAG_NONE,
                XDP_CURSOR_MODE_HIDDEN,
                XDP_PERSIST_MODE_TRANSIENT,
                session_restore_token_,
                nullptr, // cancellable
                [](GObject *obj, GAsyncResult *res, gpointer data) {
                    reinterpret_cast<PortalRemoteDesktop*>(data)->cb_init_remote_desktop_session(obj, res);
                },
                this);

    return false; // don't reschedule
}

void PortalRemoteDesktop::glib_thread()
{
    auto context = g_main_loop_get_context(glib_main_loop_);

    while (g_main_loop_is_running(glib_main_loop_)) {
        Thread::testCancel();
        g_main_context_iteration(context, true);
    }
}

} // namespace inputleap
