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

#ifndef INPUTLEAP_LIB_PLATFORM_PORTAL_INPUT_CAPTURE_H
#define INPUTLEAP_LIB_PLATFORM_PORTAL_INPUT_CAPTURE_H

#include "config.h"

#if HAVE_LIBPORTAL_INPUTCAPTURE

#include "mt/Thread.h"
#include "platform/EiScreen.h"

#include <glib.h>
#include <libportal/portal.h>

namespace inputleap {

class PortalInputCapture {
public:
    PortalInputCapture(EiScreen *screen, IEventQueue *events);
    ~PortalInputCapture();
    void enable();

private:
    void glib_thread();
    gboolean timeout_handler();
    gboolean init_input_capture_session();
    void cb_init_input_capture_session(GObject* object, GAsyncResult *res);
    void cb_set_pointer_barriers(GObject* object, GAsyncResult *res);
    void cb_session_closed(XdpSession *session);
    void cb_disabled(XdpInputCaptureSession* session);
    void cb_activated(XdpInputCaptureSession *session, GVariant *options);
    void cb_deactivated(XdpInputCaptureSession *session, GVariant *options);
    void cb_zones_changed(XdpInputCaptureSession *session, GVariant *options);

    /// g_signal_connect callback wrapper
    static void cb_session_closed_cb(XdpSession* session, gpointer data)
    {
        reinterpret_cast<PortalInputCapture*>(data)->cb_session_closed(session);
    }
    static void cb_disabled_cb(XdpInputCaptureSession *session, gpointer data)
    {
        reinterpret_cast<PortalInputCapture*>(data)->cb_disabled(session);
    }
    static void cb_activated_cb(XdpInputCaptureSession* session, GVariant* options,
                                gpointer data)
    {
        reinterpret_cast<PortalInputCapture*>(data)->cb_activated(session, options);
    }
    static void cb_deactivated_cb(XdpInputCaptureSession* session, GVariant* options,
                                  gpointer data)
    {
        reinterpret_cast<PortalInputCapture*>(data)->cb_deactivated(session, options);
    }
    static void cb_zones_changed_cb(XdpInputCaptureSession* session, GVariant* options,
                                    gpointer data)
    {
        reinterpret_cast<PortalInputCapture*>(data)->cb_zones_changed(session, options);
    }

    int fake_eis_fd();

private:
    EiScreen* screen_ = nullptr;
    IEventQueue* events_ = nullptr;

    std::unique_ptr<Thread> glib_thread_;
    GMainLoop* glib_main_loop_ = nullptr;

    XdpPortal* portal_ = nullptr;
    XdpInputCaptureSession* session_ = nullptr;

    std::vector<guint> signals_;

    bool enabled_ = false;

    std::vector<XdpInputCapturePointerBarrier*> barriers_;
};

} // namespace inputleap

#endif // HAVE_LIBPORTAL_INPUTCAPTURE
#endif // INPUTLEAP_LIB_PLATFORM_PORTAL_INPUT_CAPTURE_H
