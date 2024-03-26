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

#include "inputleap/App.h"
#include "Screen.h"

#include "base/Log.h"
#include "common/Version.h"
#include "inputleap/protocol_types.h"
#include "base/XBase.h"
#include "base/log_outputters.h"
#include "inputleap/Exceptions.h"
#include "inputleap/ArgsBase.h"
#include "ipc/IpcServerProxy.h"
#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"
#include "base/EventQueue.h"
#include "common/DataDirectories.h"

#if SYSAPI_WIN32
#include "base/IEventQueue.h"
#endif

#include <iostream>
#include <stdio.h>

#if WINAPI_CARBON
#include <ApplicationServices/ApplicationServices.h>
#endif

#if defined(__APPLE__)
#include "platform/OSXDragSimulator.h"
#endif

namespace inputleap {

App* App::s_instance = nullptr;

App::App(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver, ArgsBase* args) :
    m_bye(&exit),
    m_taskBarReceiver(nullptr),
    m_suspended(false),
    m_events(events),
    m_args(args),
    m_fileLog(nullptr),
    m_createTaskBarReceiver(createTaskBarReceiver),
    m_appUtil(events),
    m_ipcClient(nullptr),
    m_socketMultiplexer(nullptr)
{
    assert(s_instance == nullptr);
    s_instance = this;
}

App::~App()
{
    s_instance = nullptr;
    delete m_args;
}

void
App::version()
{
    std::cout << argsBase().m_exename << " " << kVersion << "\n";
    std::cout <<"Protocol version " << kProtocolMajorVersion << "." << kProtocolMinorVersion << "\n";
    std::cout << kCopyright << "\n";
}

int
App::run(int argc, char** argv)
{
#if MAC_OS_X_VERSION_10_7
    // dock hide only supported on lion :(
    ProcessSerialNumber psn = { 0, kCurrentProcess };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    GetCurrentProcess(&psn);
#pragma GCC diagnostic pop

    TransformProcessType(&psn, kProcessTransformToBackgroundApplication);
#endif

    // install application in to arch
    appUtil().adoptApp(this);

    // HACK: fail by default (saves us setting result in each catch)
    int result = kExitFailed;

    try {
        result = appUtil().run(argc, argv);
    }
    catch (XExitApp& e) {
        // instead of showing a nasty error, just exit with the error code.
        // not sure if i like this behaviour, but it's probably better than
        // using the exit(int) function!
        result = e.getCode();
    }
    catch (std::exception& e) {
        LOG_CRIT("An error occurred: %s\n", e.what());
    }
    catch (...) {
        LOG_CRIT("An unknown error occurred.\n");
    }

    appUtil().beforeAppExit();

    return result;
}

int
App::daemonMainLoop(int, const char**)
{
#if SYSAPI_WIN32
    SystemLogger sysLogger(daemonName(), false);
#else
    SystemLogger sysLogger(daemonName(), true);
#endif
    return mainLoop();
}

void
App::setupFileLogging()
{
    if (argsBase().m_logFile != nullptr) {
        m_fileLog = new FileLogOutputter(argsBase().m_logFile);
        CLOG->insert(m_fileLog);
        LOG_DEBUG1("logging to file (%s) enabled", argsBase().m_logFile);
    }
}

void
App::loggingFilterWarning()
{
    if (CLOG->getFilter() > CLOG->getConsoleMaxLevel()) {
        if (argsBase().m_logFile == nullptr) {
            LOG_WARN("log messages above %s are NOT sent to console (use file logging)",
                CLOG->getFilterName(CLOG->getConsoleMaxLevel()));
        }
    }
}

void
App::initApp(int argc, const char** argv)
{
    // parse command line
    parseArgs(argc, argv);

    inputleap::DataDirectories::profile(argsBase().m_profileDirectory);

    // set log filter
    if (!CLOG->setFilter(argsBase().m_logFilter)) {
        LOG_PRINT("%s: unrecognized log level `%s'" BYE,
            argsBase().m_exename.c_str(), argsBase().m_logFilter, argsBase().m_exename.c_str());
        m_bye(kExitArgs);
    }
    loggingFilterWarning();

    if (argsBase().m_enableDragDrop) {
        LOG_INFO("drag and drop enabled");
        if (!argsBase().m_dropTarget.empty()) {
            LOG_INFO("drop target: %s", argsBase().m_dropTarget.c_str());
        }
    }

    // setup file logging after parsing args
    setupFileLogging();

    // load configuration
    loadConfig();

    if (!argsBase().m_disableTray) {

        // create a log buffer so we can show the latest message
        // as a tray icon tooltip
        BufferedLogOutputter* logBuffer = new BufferedLogOutputter(1000);
        CLOG->insert(logBuffer, true);

        // make the task bar receiver.  the user can control this app
        // through the task bar.
        if (m_createTaskBarReceiver != nullptr)
            m_taskBarReceiver = m_createTaskBarReceiver(logBuffer, m_events);
    }
}

void
App::initIpcClient()
{
    m_ipcClient = new IpcClient(m_events, m_socketMultiplexer.get());
    m_ipcClient->connect();

    m_events->add_handler(EventType::IPC_CLIENT_MESSAGE_RECEIVED, m_ipcClient,
                          [this](const auto& event) { handle_ipc_message(event); });
}

void
App::cleanupIpcClient()
{
    m_ipcClient->disconnect();
    m_events->remove_handler(EventType::IPC_CLIENT_MESSAGE_RECEIVED, m_ipcClient);
    delete m_ipcClient;
}

void App::handle_ipc_message(const Event& e)
{
    const auto& m = e.get_data_as<IpcMessage>();
    if (m.type() == kIpcShutdown) {
        LOG_INFO("got ipc shutdown message");
        m_events->add_event(EventType::QUIT);
    }
}

void App::run_events_loop()
{
    m_events->loop();

#if defined(MAC_OS_X_VERSION_10_7)

    stopCocoaLoop();

#endif
}

//
// MinimalApp
//

MinimalApp::MinimalApp() :
    App(nullptr, nullptr, new ArgsBase())
{
    m_arch.init();
    setEvents(m_events);
}

MinimalApp::~MinimalApp()
{
}

int
MinimalApp::standardStartup(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    return 0;
}

int
MinimalApp::runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
    (void) argc;
    (void) argv;
    (void) outputter;
    (void) startup;

    return 0;
}

void
MinimalApp::startNode()
{
}

int
MinimalApp::mainLoop()
{
    return 0;
}

int
MinimalApp::foregroundStartup(int argc, char** argv)
{
    (void) argc;
    (void) argv;

    return 0;
}

std::unique_ptr<Screen> MinimalApp::create_screen()
{
    return nullptr;
}

void
MinimalApp::loadConfig()
{
}

bool MinimalApp::loadConfig(const std::string& pathname)
{
    (void) pathname;

    return false;
}

const char*
MinimalApp::daemonInfo() const
{
    return "";
}

const char*
MinimalApp::daemonName() const
{
    return "";
}

void
MinimalApp::parseArgs(int argc, const char* const* argv)
{
    (void) argc;
    (void) argv;
}

} // namespace inputleap
