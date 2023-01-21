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

#ifndef INPUTLEAP_LIB_PLATFORM_PORTAL_REMOTE_DESKTOP_H
#define INPUTLEAP_LIB_PLATFORM_PORTAL_REMOTE_DESKTOP_H

#include "config.h"

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/portal.h>

namespace inputleap {

class PortalRemoteDesktop {
public:
    PortalRemoteDesktop(EiScreen *screen, IEventQueue *events);
    ~PortalRemoteDesktop();

private:
    void glib_thread();
    gboolean timeout_handler();
    gboolean init_remote_desktop_session();
    void cb_init_remote_desktop_session(GObject* object, GAsyncResult* res);
    void cb_session_started(GObject* object, GAsyncResult* res);
    void cb_session_closed(XdpSession* session);

    /// g_signal_connect callback wrapper
    static void cb_session_closed_cb(XdpSession* session, gpointer data)
    {
        reinterpret_cast<PortalRemoteDesktop*>(data)->cb_session_closed(session);
    }

    int fake_eis_fd();

private:
    EiScreen* screen_;
    IEventQueue* events_;

    Thread* glib_thread_;
    GMainLoop* glib_main_loop_;

    XdpPortal* portal_;
    XdpSession* session_;

    guint session_signal_id_;
};

} // namespace inputleap

#endif // INPUTLEAP_LIB_PLATFORM_PORTAL_REMOTE_DESKTOP_H
