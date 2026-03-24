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

#pragma once

#include "config.h"

#include "mt/Thread.h"
#include "platform/EiScreen.h"
#include "inputleap/Clipboard.h"
#include "inputleap/clipboard_types.h"

#include <glib.h>
#include <gio/gio.h>
#include <libportal/portal.h>

#include <mutex>
#include <string>
#include <vector>

#if !HAVE_LIBPORTAL_OUTPUT_NONE
// Added in libportal ad82a74 Jun 2022, not yet released in libportal 0.6
// should be used as a patch on ≤ 0.6, and non-git
#define XDP_OUTPUT_NONE (XdpOutputType)0
#endif

namespace inputleap {

class PortalRemoteDesktop {
public:
    /// @param clipboard_only If true, skip EIS connection (used on primary screen
    ///        where InputCapture handles input but we need RemoteDesktop for clipboard)
    PortalRemoteDesktop(EiScreen *screen, IEventQueue *events, bool clipboard_only = false);
    ~PortalRemoteDesktop();

    /// Clipboard support via org.freedesktop.portal.Clipboard
    bool getClipboard(ClipboardID id, IClipboard* clipboard);
    bool setClipboard(ClipboardID id, const IClipboard* clipboard);

private:
    void glib_thread();
    gboolean timeout_handler();
    gboolean init_remote_desktop_session();
    void cb_init_remote_desktop_session(GObject* object, GAsyncResult* res);
    void cb_session_started(GObject* object, GAsyncResult* res);
    void cb_session_closed(XdpSession* session);
    void reconnect(unsigned int timeout=1000);

    /// g_signal_connect callback wrapper
    static void cb_session_closed_cb(XdpSession* session, gpointer data)
    {
        reinterpret_cast<PortalRemoteDesktop*>(data)->cb_session_closed(session);
    }

    int fake_eis_fd();

    // Clipboard via XDG Portal DBus
    void request_clipboard();
    void cleanup_clipboard();

    void on_selection_owner_changed(GDBusConnection* connection,
                                    const gchar* sender_name,
                                    const gchar* object_path,
                                    const gchar* interface_name,
                                    const gchar* signal_name,
                                    GVariant* parameters);
    void on_selection_transfer(GDBusConnection* connection,
                               const gchar* sender_name,
                               const gchar* object_path,
                               const gchar* interface_name,
                               const gchar* signal_name,
                               GVariant* parameters);

    static void cb_selection_owner_changed(GDBusConnection* connection,
                                           const gchar* sender_name,
                                           const gchar* object_path,
                                           const gchar* interface_name,
                                           const gchar* signal_name,
                                           GVariant* parameters,
                                           gpointer user_data)
    {
        reinterpret_cast<PortalRemoteDesktop*>(user_data)
            ->on_selection_owner_changed(connection, sender_name, object_path,
                                         interface_name, signal_name, parameters);
    }

    static void cb_selection_transfer(GDBusConnection* connection,
                                      const gchar* sender_name,
                                      const gchar* object_path,
                                      const gchar* interface_name,
                                      const gchar* signal_name,
                                      GVariant* parameters,
                                      gpointer user_data)
    {
        reinterpret_cast<PortalRemoteDesktop*>(user_data)
            ->on_selection_transfer(connection, sender_name, object_path,
                                    interface_name, signal_name, parameters);
    }

    std::string read_mime_type_from_portal(const std::string& mime_type);

private:
    EiScreen* screen_;
    IEventQueue* events_;

    Thread* glib_thread_;
    GMainLoop* glib_main_loop_ = nullptr;

    XdpPortal* portal_ = nullptr;
    XdpSession* session_ = nullptr;
    char *session_restore_token_ = nullptr;

    guint session_signal_id_ = 0;
    guint session_iteration_ = 0; /// The number of successful sessions we've had already
    bool clipboard_only_ = false;

    // Clipboard state
    GDBusConnection* dbus_connection_ = nullptr;
    std::string session_handle_;
    guint selection_owner_changed_sub_ = 0;
    guint selection_transfer_sub_ = 0;
    std::mutex clipboard_mutex_;
    Clipboard stored_clipboard_;
    std::vector<std::string> available_mime_types_;
    bool clipboard_enabled_ = false;
    bool we_own_clipboard_ = false;
};

} // namespace inputleap
