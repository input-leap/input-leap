/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
 *
 * This package is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * found in the file LICENSE that should have accompanied this file.
 *
 * This package is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "inputleap/ClientApp.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueue.h"
#include "config.h"

#if WINAPI_MSWINDOWS
#include "MSWindowsClientTaskBarReceiver.h"
#endif

namespace inputleap {

#if WINAPI_XWINDOWS || WINAPI_LIBEI || WINAPI_CARBON
CreateTaskBarReceiverFunc createTaskBarReceiver = nullptr;
#endif

int client_main(int argc, char** argv) {
#if SYSAPI_WIN32
    // record window instance for tray icon, etc
    ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));
#endif

    Arch arch;
    arch.init();

    Log log;
    EventQueue events;

    // TODO: Remove once Wayland support is stabilised.

    // This block only warns when `libportal` and `libeis` aren't available - as
    // well as if the top-level CMake option is enabled - by default, it is.

    // It serves as a way to warn users that using Wayland on platforms matching
    // this boolean expression, that their platform might not be fully
    // supported.

    // It does *not* run on X11 platforms, Win32, or macOS.
#if (defined(WINAPI_LIBEI) && defined(INPUTLEAP_WARN_ON_WAYLAND)) && (!defined(HAVE_LIBPORTAL_INPUTCAPTURE) || !defined(HAVE_LIBPORTAL_SESSION_CONNECT_TO_EIS))
    const char *val = std::getenv("WAYLAND_DISPLAY");
    if (val == nullptr) {
        // Return, we're not running on Wayland. Possibly. Could be enhanced.
        return
    } else {
        // We are running on Wayland.
        // TODO: Add log message.
    };
#endif

    ClientApp app(&events, createTaskBarReceiver);
    int result = app.run(argc, argv);
#if SYSAPI_WIN32
    if (IsDebuggerPresent()) {
        printf("\n\nHit a key to close...\n");
        getchar();
    }
#endif
    return result;
}

} // namespace inputleap

int main(int argc, char** argv)
{
    return inputleap::client_main(argc, argv);
}
