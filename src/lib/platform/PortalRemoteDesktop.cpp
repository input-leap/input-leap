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
    portal_(xdp_portal_new()),
    session_(nullptr)
{
    glib_main_loop_ = g_main_loop_new(NULL, true);
    glib_thread_ = new Thread([this](){ glib_thread(); });

    auto init_cb = [](gpointer data) -> gboolean
    {
        return reinterpret_cast<PortalRemoteDesktop*>(data)->init_remote_desktop_session();
    };

    g_idle_add(init_cb, this);
}

PortalRemoteDesktop::~PortalRemoteDesktop()
{
    if (g_main_loop_is_running(glib_main_loop_))
        g_main_loop_quit(glib_main_loop_);

    if (glib_thread_ != nullptr) {
        glib_thread_->cancel();
        glib_thread_->wait();
        delete glib_thread_;
        glib_thread_ = NULL;

        g_main_loop_unref(glib_main_loop_);
        glib_main_loop_ = NULL;
    }

    if (session_signal_id_)
        g_signal_handler_disconnect(session_, session_signal_id_);
    if (session_ != nullptr)
        g_object_unref(session_);
    g_object_unref(portal_);
}

gboolean PortalRemoteDesktop::timeout_handler()
{
    return true; // keep re-triggering
}

int PortalRemoteDesktop::fake_eis_fd()
{
    auto path = std::getenv("LIBEI_SOCKET");

    if (!path) {
        LOG((CLOG_DEBUG "Cannot fake EIS socket, LIBEI_SOCKET environment variable is unset"));
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
    if (result != 0)
        LOG((CLOG_DEBUG "Faked EIS fd failed: %s", strerror(errno)));

    return sock;
}

void PortalRemoteDesktop::cb_session_closed(XdpSession* session)
{
    LOG((CLOG_ERR "Our RemoteDesktop session was closed, exiting."));
    g_main_loop_quit(glib_main_loop_);
    events_->add_event(EventType::QUIT);

    g_signal_handler_disconnect(session, session_signal_id_);
}

void PortalRemoteDesktop::cb_session_started(GObject* object, GAsyncResult* res)
{
    g_autoptr(GError) error = NULL;
    auto session = XDP_SESSION(object);
    auto success = xdp_session_start_finish(session, res, &error);
    if (!success) {
        LOG((CLOG_ERR "Failed to start session"));
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
    }

    // ConnectToEIS requires version 2 of the xdg-desktop-portal (and the same
    // version in the impl.portal), i.e. you'll need an updated compositor on
    // top of everything...
    auto fd = -1;
#if HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS
    fd = xdp_session_connect_to_eis(session);
#endif
    if (fd < 0) {
        if (fd == -ENOTSUP)
            LOG((CLOG_ERR "The XDG desktop portal does not support EI.", strerror(-fd)));
        else
            LOG((CLOG_ERR "Failed to connect to EIS: %s", strerror(-fd)));

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
}

void PortalRemoteDesktop::cb_init_remote_desktop_session(GObject* object, GAsyncResult* res)
{
    LOG((CLOG_DEBUG "Session ready"));
    g_autoptr(GError) error = NULL;

    auto session = xdp_portal_create_remote_desktop_session_finish(XDP_PORTAL(object), res, &error);
    if (!session) {
        LOG((CLOG_ERR "Failed to initialize RemoteDesktop session, quitting: %s", error->message));
        g_main_loop_quit(glib_main_loop_);
        events_->add_event(EventType::QUIT);
        return;
    }

    session_ = session;

    // FIXME: the lambda trick doesn't work here for unknown reasons, we need
    // the static function
    session_signal_id_ = g_signal_connect(G_OBJECT(session), "closed",
                                         G_CALLBACK(cb_session_closed_cb),
                                         this);

    xdp_session_start(session,
                      nullptr, // parent
                      nullptr, // cancellable
                      [](GObject *obj, GAsyncResult *res, gpointer data) {
                          reinterpret_cast<PortalRemoteDesktop*>(data)->cb_session_started(obj, res);
                      },
                      this);
}

gboolean PortalRemoteDesktop::init_remote_desktop_session()
{
    LOG((CLOG_DEBUG "Setting up the RemoteDesktop session"));
    xdp_portal_create_remote_desktop_session(
                portal_,
                static_cast<XdpDeviceType>(XDP_DEVICE_POINTER | XDP_DEVICE_KEYBOARD),
                XDP_OUTPUT_NONE,
                XDP_REMOTE_DESKTOP_FLAG_NONE,
                XDP_CURSOR_MODE_HIDDEN,
                nullptr, // cancellable
                [](GObject *obj, GAsyncResult *res, gpointer data) {
                    reinterpret_cast<PortalRemoteDesktop*>(data)->cb_init_remote_desktop_session(obj, res);
                },
                this);

    return false;
}

void PortalRemoteDesktop::glib_thread()
{
    auto context = g_main_loop_get_context(glib_main_loop_);

    LOG((CLOG_DEBUG "GLib thread running"));

    while (g_main_loop_is_running(glib_main_loop_)) {
        Thread::testCancel();

        if (g_main_context_iteration(context, true)) {
            LOG((CLOG_DEBUG "Glib events!!"));
        }
    }

    LOG((CLOG_DEBUG "Shutting down GLib thread"));
}

} // namespace inputleap
