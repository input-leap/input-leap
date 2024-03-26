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

#include "client/Client.h"
#include "inputleap/ArgParser.h"
#include "PlatformScreenLoggingWrapper.h"
#include "inputleap/protocol_types.h"
#include "inputleap/Screen.h"
#include "inputleap/XScreen.h"
#include "inputleap/ClientArgs.h"
#include "net/NetworkAddress.h"
#include "net/TCPSocketFactory.h"
#include "net/SocketMultiplexer.h"
#include "net/XSocket.h"
#include "mt/Thread.h"
#include "arch/IArchTaskBarReceiver.h"
#include "arch/Arch.h"
#include "base/String.h"
#include "base/Event.h"
#include "base/EventQueueTimer.h"
#include "base/IEventQueue.h"
#include "base/log_outputters.h"
#include "base/EventQueue.h"
#include "base/Log.h"
#include "common/Version.h"

#if WINAPI_MSWINDOWS
#include "platform/MSWindowsScreen.h"
#endif
#if WINAPI_XWINDOWS
#include "platform/XWindowsScreen.h"
#endif
#if WINAPI_LIBEI
#include "platform/EiScreen.h"
#endif
#if WINAPI_CARBON
#include "platform/OSXScreen.h"
#endif

#if defined(__APPLE__)
#include "platform/OSXDragSimulator.h"
#endif

#include <iostream>
#include <stdio.h>
#include <sstream>

#define RETRY_TIME 1.0

namespace inputleap {

ClientApp::ClientApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver) :
    App(events, createTaskBarReceiver, new ClientArgs()),
    m_client(nullptr),
    m_clientScreen(nullptr),
    m_serverAddress(nullptr)
{
}

ClientApp::~ClientApp()
{
}

void
ClientApp::parseArgs(int argc, const char* const* argv)
{
    ArgParser argParser(this);
    bool result = argParser.parseClientArgs(args(), argc, argv);

    if (!result || args().m_shouldExit) {
        m_bye(kExitArgs);
    }
    else {
        // save server address
        if (!args().network_address.empty()) {
            try {
                *m_serverAddress = NetworkAddress(args().network_address, kDefaultPort);
                m_serverAddress->resolve();
            }
            catch (XSocketAddress& e) {
                // allow an address that we can't look up if we're restartable.
                // we'll try to resolve the address each time we connect to the
                // server.  a bad port will never get better.  patch by Brent
                // Priddy.
                if (!args().m_restartable || e.getError() == XSocketAddress::kBadPort) {
                    LOG_PRINT("%s: %s" BYE,
                        args().m_exename.c_str(), e.what(), args().m_exename.c_str());
                    m_bye(kExitFailed);
                }
            }
        }
    }
}

void
ClientApp::help()
{
    std::ostringstream buffer;
    buffer << "Start the InputLeap client and connect to a remote server component.\n"
           << "\n"
           << "Usage: " << args().m_exename << " [--yscroll <delta>]"
#ifdef WINAPI_XWINDOWS
           << " [--use-x11] [--display <display>]"
#endif
#ifdef WINAPI_LIBEI
           << " [--use-ei]"
#endif
           << HELP_SYS_ARGS
           << HELP_COMMON_ARGS << " <server-address>\n"
           << "\n"
           << "Options:\n"
           << HELP_COMMON_INFO_1
#if WINAPI_XWINDOWS
           << "      --use-x11            use the X11 backend\n"
           << "      --display <display>  connect to the X server at <display>\n"
#endif
#ifdef WINAPI_LIBEI
           << "      --use-ei             use the EI backend\n"
           << "      --disable-portal     do not use the org.freedesktop.portal.RemoteDesktop portal,\n"
           << "                           connect to $LIBEI_SOCKET directly\n"
#endif
           << HELP_SYS_INFO
           << "      --yscroll <delta>    defines the vertical scrolling delta, which is\n"
           << "                           120 by default.\n"
           << HELP_COMMON_INFO_2
           << "\n"
           << "Default options are marked with a *\n"
           << "\n"
           << "The server address is of the form: [<hostname>][:<port>]. The hostname\n"
           << "must be the address or hostname of the server. Placing brackets around\n"
           << "an IPv6 address is required when also specifying a port number and \n"
           << "optional otherwise. The default port number is " << kDefaultPort << ".\n";

    LOG_PRINT("%s", buffer.str().c_str());
}

const char*
ClientApp::daemonName() const
{
#if SYSAPI_WIN32
    return "InputLeap Client";
#elif SYSAPI_UNIX
    return "input-leapc";
#endif
}

const char*
ClientApp::daemonInfo() const
{
#if SYSAPI_WIN32
    return "Allows another computer to share it's keyboard and mouse with this computer.";
#elif SYSAPI_UNIX
    return "";
#endif
}

std::unique_ptr<Screen> ClientApp::create_screen()
{
    auto plat_screen = create_platform_screen();
    if (Log::getInstance()->getFilter() >= kDEBUG2) {
        plat_screen = std::make_unique<PlatformScreenLoggingWrapper>(std::move(plat_screen));
    }
    return std::make_unique<Screen>(std::move(plat_screen), m_events);
}

void
ClientApp::updateStatus()
{
    updateStatus("");
}


void ClientApp::updateStatus(const std::string& msg)
{
    if (m_taskBarReceiver)
    {
        m_taskBarReceiver->updateStatus(m_client, msg);
    }
}


void
ClientApp::resetRestartTimeout()
{
    // retry time can nolonger be changed
    //s_retryTime = 0.0;
}


double
ClientApp::nextRestartTimeout()
{
    // retry at a constant rate (Issue 52)
    return RETRY_TIME;

    /*
    // choose next restart timeout.  we start with rapid retries
    // then slow down.
    if (s_retryTime < 1.0) {
    s_retryTime = 1.0;
    }
    else if (s_retryTime < 3.0) {
    s_retryTime = 3.0;
    }
    else {
    s_retryTime = 5.0;
    }
    return s_retryTime;
    */
}


void ClientApp::handle_screen_error()
{
    LOG_CRIT("error on screen");
    m_events->add_event(EventType::QUIT);
}


std::unique_ptr<Screen> ClientApp::open_client_screen()
{
    auto screen = create_screen();
    if (!argsBase().m_dropTarget.empty()) {
        screen->setDropTarget(argsBase().m_dropTarget);
    }
    screen->setEnableDragDrop(argsBase().m_enableDragDrop);
    m_events->add_handler(EventType::SCREEN_ERROR, screen->get_event_target(),
                          [this](const auto& e){ handle_screen_error(); });
    return screen;
}

void
ClientApp::handle_client_restart(const Event&, EventQueueTimer* timer)
{
    // discard old timer
    m_events->remove_handler(EventType::TIMER, timer);
    m_events->deleteTimer(timer);

    // reconnect
    startClient();
}


void
ClientApp::scheduleClientRestart(double retryTime)
{
    // install a timer and handler to retry later
    LOG_DEBUG("retry in %.0f seconds", retryTime);
    EventQueueTimer* timer = m_events->newOneShotTimer(retryTime, nullptr);
    m_events->add_handler(EventType::TIMER, timer,
                          [this, timer](const Event& event) { handle_client_restart(event, timer); });
}


void ClientApp::handle_client_connected()
{
    // using CLOG_PRINT here allows the GUI to see that the client is connected
    // regardless of which log level is set
    LOG_PRINT("connected to server");
    resetRestartTimeout();
    updateStatus();
}


void ClientApp::handle_client_failed(const Event& e)
{
    const auto& info = e.get_data_as<Client::FailInfo>();

    updateStatus(std::string("Failed to connect to server: ") + info.m_what);
    if (!args().m_restartable || !info.m_retry) {
        LOG_ERR("failed to connect to server: %s", info.m_what.c_str());
        m_events->add_event(EventType::QUIT);
    }
    else {
        LOG_WARN("failed to connect to server: %s", info.m_what.c_str());
        if (!m_suspended) {
            scheduleClientRestart(nextRestartTimeout());
        }
    }
}


void ClientApp::handle_client_disconnected()
{
    LOG_NOTE("disconnected from server");
    if (!args().m_restartable) {
        m_events->add_event(EventType::QUIT);
    }
    else if (!m_suspended) {
        scheduleClientRestart(nextRestartTimeout());
    }
    updateStatus();
}

Client* ClientApp::openClient(const std::string& name, const NetworkAddress& address,
                              inputleap::Screen* screen)
{
    Client* client = new Client(
        m_events,
        name,
        address,
        new TCPSocketFactory(m_events, getSocketMultiplexer()),
        screen,
        args());

    try {
        m_events->add_handler(EventType::CLIENT_CONNECTED, client->get_event_target(),
                              [this](const auto& e) { handle_client_connected(); });
        m_events->add_handler(EventType::CLIENT_CONNECTION_FAILED, client->get_event_target(),
                              [this](const auto& e) { handle_client_failed(e); });
        m_events->add_handler(EventType::CLIENT_DISCONNECTED, client->get_event_target(),
                              [this](const auto& e) { handle_client_disconnected(); });

    } catch (std::bad_alloc &ba) {
        delete client;
        throw ba;
    }

    return client;
}


void
ClientApp::closeClient(Client* client)
{
    if (client == nullptr) {
        return;
    }

    m_events->remove_handler(EventType::CLIENT_CONNECTED, client);
    m_events->remove_handler(EventType::CLIENT_CONNECTION_FAILED, client);
    m_events->remove_handler(EventType::CLIENT_DISCONNECTED, client);
    delete client;
}

int
ClientApp::foregroundStartup(int argc, char** argv)
{
    initApp(argc, argv);

    // never daemonize
    return mainLoop();
}

bool
ClientApp::startClient()
{
    double retryTime;
    std::unique_ptr<Screen> client_screen;
    try {
        if (!m_clientScreen) {
            client_screen = open_client_screen();
            m_client = openClient(args().m_name, *m_serverAddress, client_screen.get());
            m_clientScreen = std::move(client_screen);
            LOG_NOTE("started client");
        }

        m_client->connect();

        updateStatus();
        return true;
    }
    catch (XScreenUnavailable& e) {
        LOG_WARN("secondary screen unavailable: %s", e.what());
        updateStatus(std::string("secondary screen unavailable: ") + e.what());
        m_clientScreen.reset();
        retryTime = e.getRetryTime();
    }
    catch (XScreenOpenFailure& e) {
        LOG_CRIT("failed to start client: %s", e.what());
        m_clientScreen.reset();
        return false;
    }
    catch (XBase& e) {
        LOG_CRIT("failed to start client: %s", e.what());
        m_clientScreen.reset();
        return false;
    }

    if (args().m_restartable) {
        scheduleClientRestart(retryTime);
        return true;
    }
    else {
        // don't try again
        return false;
    }
}


void
ClientApp::stopClient()
{
    closeClient(m_client);
    m_client = nullptr;
    m_clientScreen.reset();
}


int
ClientApp::mainLoop()
{
    // create socket multiplexer.  this must happen after daemonization
    // on unix because threads evaporate across a fork().
    setSocketMultiplexer(std::make_unique<SocketMultiplexer>());

    // start client, etc
    appUtil().startNode();

    // init ipc client after node start, since create a new screen wipes out
    // the event queue (the screen ctors call set_buffer).
    if (argsBase().m_enableIpc) {
        initIpcClient();
    }

    // run event loop.  if startClient() failed we're supposed to retry
    // later.  the timer installed by startClient() will take care of
    // that.
    DAEMON_RUNNING(true);

#if defined(MAC_OS_X_VERSION_10_7)

    Thread thread([this](){ run_events_loop(); });

    // wait until carbon loop is ready
    OSXScreen* screen = dynamic_cast<OSXScreen*>(
        m_clientScreen->getPlatformScreen());
    screen->waitForCarbonLoop();

    runCocoaApp();
#else
    m_events->loop();
#endif

    DAEMON_RUNNING(false);

    // close down
    LOG_DEBUG1("stopping client");
    stopClient();
    updateStatus();
    LOG_NOTE("stopped client");

    if (argsBase().m_enableIpc) {
        cleanupIpcClient();
    }

    return kExitSuccess;
}

static
int
daemonMainLoopStatic(int argc, const char** argv)
{
    return ClientApp::instance().daemonMainLoop(argc, argv);
}

int
ClientApp::standardStartup(int argc, char** argv)
{
    initApp(argc, argv);

    // daemonize if requested
    if (args().m_daemon) {
        return ARCH->daemonize(daemonName(), &daemonMainLoopStatic);
    }
    else {
        return mainLoop();
    }
}

int
ClientApp::runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
    // general initialization
    m_serverAddress = new NetworkAddress;
    argsBase().m_exename = ArgParser::parse_exename(argv[0]);

    // install caller's output filter
    if (outputter != nullptr) {
        CLOG->insert(outputter);
    }

    int result;
    try
    {
        // run
        result = startup(argc, argv);
    }
    catch (...)
    {
        if (m_taskBarReceiver)
        {
            // done with task bar receiver
            delete m_taskBarReceiver;
        }

        delete m_serverAddress;

        throw;
    }

    return result;
}

void
ClientApp::startNode()
{
    // start the client.  if this return false then we've failed and
    // we shouldn't retry.
    LOG_DEBUG1("starting client");
    if (!startClient()) {
        m_bye(kExitFailed);
    }
}

std::unique_ptr<IPlatformScreen> ClientApp::create_platform_screen()
{
#if WINAPI_MSWINDOWS
    return std::make_unique<MSWindowsScreen>(false, args().m_noHooks, args().m_stopOnDeskSwitch,
                                             m_events);
#endif
#if WINAPI_LIBEI
    if (args().use_ei) {
        return std::make_unique<EiScreen>(false, m_events, args().use_portal);
    }
#endif
#if WINAPI_XWINDOWS
    if (args().use_x11) {
        return std::make_unique<XWindowsScreen>(new XWindowsImpl(), args().m_display, false,
                                                args().m_yscroll, m_events);
    }
#endif
#if WINAPI_CARBON
    return std::make_unique<OSXScreen>(m_events, false);
#endif
    throw std::runtime_error("Failed to create screen, this shouldn't happen");
}

} // namespace inputleap
