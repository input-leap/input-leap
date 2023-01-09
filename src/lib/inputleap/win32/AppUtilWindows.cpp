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

#include "inputleap/win32/AppUtilWindows.h"
#include "inputleap/Screen.h"
#include "inputleap/ArgsBase.h"
#include "inputleap/App.h"
#include "inputleap/XBarrier.h"
#include "platform/MSWindowsScreen.h"
#include "arch/win32/XArchWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/IArchTaskBarReceiver.h"
#include "base/Log.h"
#include "base/log_outputters.h"
#include "base/IEventQueue.h"
#include "base/Event.h"
#include "base/EventQueue.h"
#include "base/Time.h"
#include "common/Version.h"

#include <sstream>
#include <iostream>
#include <conio.h>
#include <VersionHelpers.h>

namespace inputleap {

AppUtilWindows::AppUtilWindows(IEventQueue* events) :
    m_events(events),
    m_exitMode(kExitModeNormal)
{
    if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE) == FALSE)
    {
        throw std::runtime_error(error_code_to_string_windows(GetLastError()));
    }
}

AppUtilWindows::~AppUtilWindows()
{
}

BOOL WINAPI AppUtilWindows::consoleHandler(DWORD)
{
    LOG((CLOG_INFO "got shutdown signal"));
    IEventQueue* events = AppUtil::instance().app().getEvents();
    events->add_event(Event(EventType::QUIT));
    return TRUE;
}

static
int
mainLoopStatic()
{
    return AppUtil::instance().app().mainLoop();
}

int
AppUtilWindows::daemonNTMainLoop(int argc, const char** argv)
{
    app().initApp(argc, argv);
    debugServiceWait();

    // NB: what the hell does this do?!
    app().argsBase().m_backend = false;

    return ArchMiscWindows::runDaemon(mainLoopStatic);
}

void
AppUtilWindows::exitApp(int code)
{
    switch (m_exitMode) {

        case kExitModeDaemon:
            ArchMiscWindows::daemonFailed(code);
            break;

        default:
            throw XExitApp(code);
    }
}

int daemonNTMainLoopStatic(int argc, const char** argv)
{
    return AppUtilWindows::instance().daemonNTMainLoop(argc, argv);
}

int
AppUtilWindows::daemonNTStartup(int, char**)
{
    SystemLogger sysLogger(app().daemonName(), false);
    m_exitMode = kExitModeDaemon;
    return ARCH->daemonize(app().daemonName(), daemonNTMainLoopStatic);
}

static
int
daemonNTStartupStatic(int argc, char** argv)
{
    return AppUtilWindows::instance().daemonNTStartup(argc, argv);
}

static
int
foregroundStartupStatic(int argc, char** argv)
{
    return AppUtil::instance().app().foregroundStartup(argc, argv);
}

void
AppUtilWindows::beforeAppExit()
{
    // this can be handy for debugging, since the application is launched in
    // a new console window, and will normally close on exit (making it so
    // that we can't see error messages).
    if (app().argsBase().m_pauseOnExit) {
        std::cout << "\n" << "press any key to exit...\n";
        int c = _getch();
    }
}

int
AppUtilWindows::run(int argc, char** argv)
{
    if (!IsWindowsXPSP3OrGreater()) {
        throw std::runtime_error("Barrier only supports Windows XP SP3 and above.");
    }

    // record window instance for tray icon, etc
    ArchMiscWindows::setInstanceWin32(GetModuleHandle(nullptr));

    MSWindowsScreen::init(ArchMiscWindows::instanceWin32());
    Thread::getCurrentThread().setPriority(-14);

    StartupFunc startup;
    if (ArchMiscWindows::wasLaunchedAsService()) {
        startup = &daemonNTStartupStatic;
    } else {
        startup = &foregroundStartupStatic;
        app().argsBase().m_daemon = false;
    }

    return app().runInner(argc, argv, nullptr, startup);
}

AppUtilWindows&
AppUtilWindows::instance()
{
    return (AppUtilWindows&)AppUtil::instance();
}

void
AppUtilWindows::debugServiceWait()
{
    if (app().argsBase().m_debugServiceWait)
    {
        while(true)
        {
            // this code is only executed when the process is launched via the
            // windows service controller (and --debug-service-wait arg is
            // used). to debug, set a breakpoint on this line so that
            // execution is delayed until the debugger is attached.
            inputleap::this_thread_sleep(1);
            LOG((CLOG_INFO "waiting for debugger to attach"));
        }
    }
}

void
AppUtilWindows::startNode()
{
    app().startNode();
}

} // namespace inputleap
