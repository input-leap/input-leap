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

#include "inputleap/ServerApp.h"

#include "server/Server.h"
#include "server/ClientListener.h"
#include "server/ClientProxy.h"
#include "server/PrimaryClient.h"
#include "inputleap/ArgParser.h"
#include "inputleap/Screen.h"
#include "inputleap/XScreen.h"
#include "inputleap/ServerTaskBarReceiver.h"
#include "inputleap/ServerArgs.h"
#include "net/SocketMultiplexer.h"
#include "net/TCPSocketFactory.h"
#include "net/XSocket.h"
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/log_outputters.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "common/Version.h"
#include "common/DataDirectories.h"

#if SYSAPI_WIN32
#include "arch/win32/ArchMiscWindows.h"
#endif

#if WINAPI_MSWINDOWS
#include "platform/MSWindowsScreen.h"
#endif
#if WINAPI_XWINDOWS
#include <unistd.h>
#include <signal.h>
#include "platform/XWindowsScreen.h"
#endif
#if WINAPI_CARBON
#include "platform/OSXScreen.h"
#endif

#if defined(__APPLE__)
#include "platform/OSXDragSimulator.h"
#endif

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <sstream>

namespace inputleap {

ServerApp::ServerApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver) :
    App(events, createTaskBarReceiver, new ServerArgs()),
    m_server(nullptr),
    m_serverState(kUninitialized),
    m_serverScreen(nullptr),
    m_primaryClient(nullptr),
    m_listener(nullptr),
    m_timer(nullptr),
    listen_address_(nullptr)
{
}

ServerApp::~ServerApp()
{
}

void
ServerApp::parseArgs(int argc, const char* const* argv)
{
    ArgParser argParser(this);
    bool result = argParser.parseServerArgs(args(), argc, argv);

    if (!result || args().m_shouldExit) {
        m_bye(kExitArgs);
    }
    else {
        if (!args().network_address.empty()) {
            try {
                *listen_address_ = NetworkAddress(args().network_address, kDefaultPort);
                listen_address_->resolve();
            }
            catch (XSocketAddress& e) {
                LOG((CLOG_PRINT "%s: %s" BYE,
                    args().m_exename.c_str(), e.what(), args().m_exename.c_str()));
                m_bye(kExitArgs);
            }
        }
    }
}

void
ServerApp::help()
{
    // refer to custom profile directory even if not saved yet
    inputleap::fs::path profile_path = argsBase().m_profileDirectory;
    if (profile_path.empty()) {
        profile_path = inputleap::DataDirectories::profile();
    }

    auto usr_config_path = (profile_path / inputleap::fs::u8path(USR_CONFIG_NAME)).u8string();
    auto sys_config_path = (inputleap::DataDirectories::systemconfig() /
                            inputleap::fs::u8path(SYS_CONFIG_NAME)).u8string();

    std::ostringstream buffer;
    buffer << "Start the InputLeap server component. The server shares the keyboard &\n"
           << "mouse of the local machine with the connected clients based on the\n"
           << "configuration file.\n"
           << "\n"
           << "Usage: " << args().m_exename
           << " [--address <address>]"
           << " [--config <pathname>]"
#ifdef WINAPI_XWINDOWS
           << " [--display <display>]"
#endif
           << HELP_SYS_ARGS
           << HELP_COMMON_ARGS
           << "\n"
           << "\n"
           << "Options:\n"
           << "  -a, --address <address>  listen for clients on the given address.\n"
           << "  -c, --config <pathname>  use the named configuration file instead.\n"
           << HELP_COMMON_INFO_1
           << "      --disable-client-cert-checking disable client SSL certificate \n"
              "                                     checking (deprecated)\n"
#ifdef WINAPI_XWINDOWS
           << "      --display <display>  connect to the X server at <display>\n"
           << "      --screen-change-script <path>\n"
           << "                           full path to script to run on screen change\n"
           << "                           first argument is the new screen name\n"
#endif
           << HELP_SYS_INFO
           << HELP_COMMON_INFO_2
           << "\n"
           << "Default options are marked with a *\n"
           << "\n"
           << "The argument for --address is of the form: [<hostname>][:<port>].  The\n"
           << "hostname must be the address or hostname of an interface on the system.\n"
           << "Placing brackets around an IPv6 address is required when also specifying \n"
           << "a port number and optional otherwise. The default is to listen on all\n"
           << "interfaces using port number " << kDefaultPort << ".\n"
           << "\n"
           << "If no configuration file pathname is provided then the first of the\n"
           << "following to load successfully sets the configuration:\n"
           << "  " << usr_config_path << "\n"
           << "  " << sys_config_path << "\n";

    LOG((CLOG_PRINT "%s", buffer.str().c_str()));
}

void
ServerApp::reloadSignalHandler(Arch::ESignal, void*)
{
    IEventQueue* events = App::instance().getEvents();
    events->add_event(EventType::SERVER_APP_RELOAD_CONFIG, events->getSystemTarget());
}

void ServerApp::reload_config()
{
    LOG((CLOG_DEBUG "reload configuration"));
    if (loadConfig(args().m_configFile)) {
        if (m_server != nullptr) {
            m_server->setConfig(*args().m_config);
        }
        LOG((CLOG_NOTE "reloaded configuration"));
    }
}

void
ServerApp::loadConfig()
{
    bool loaded = false;

    // load the config file, if specified
    if (!args().m_configFile.empty()) {
        loaded = loadConfig(args().m_configFile);
    }

    // load the default configuration if no explicit file given
    else {
        auto path = inputleap::DataDirectories::profile();
        if (!path.empty()) {
            // complete path
            path /= inputleap::fs::u8path(USR_CONFIG_NAME);

            // now try loading the user's configuration
            if (loadConfig(path.u8string())) {
                loaded            = true;
                args().m_configFile = path.u8string();
            }
        }
        if (!loaded) {
            // try the system-wide config file
            path = inputleap::DataDirectories::systemconfig();
            if (!path.empty()) {
                path /= inputleap::fs::u8path(SYS_CONFIG_NAME);
                if (loadConfig(path.u8string())) {
                    loaded            = true;
                    args().m_configFile = path.u8string();
                }
            }
        }
    }

    if (!loaded) {
        LOG((CLOG_PRINT "%s: no configuration available", args().m_exename.c_str()));
        m_bye(kExitConfig);
    }
}

bool ServerApp::loadConfig(const std::string& pathname)
{
    try {
        // load configuration
        LOG((CLOG_DEBUG "opening configuration \"%s\"", pathname.c_str()));
        std::ifstream configStream(pathname.c_str());
        if (!configStream.is_open()) {
            // report failure to open configuration as a debug message
            // since we try several paths and we expect some to be
            // missing.
            LOG((CLOG_DEBUG "cannot open configuration \"%s\"",
                pathname.c_str()));
            return false;
        }
        configStream >> *args().m_config;
        LOG((CLOG_DEBUG "configuration read successfully"));
        return true;
    }
    catch (XConfigRead& e) {
        // report error in configuration file
        LOG((CLOG_ERR "cannot read configuration \"%s\": %s",
            pathname.c_str(), e.what()));
    }
    return false;
}

void ServerApp::force_reconnect()
{
    if (m_server != nullptr) {
        m_server->disconnect();
    }
}

void ServerApp::handle_client_connected(const Event&, ClientListener* listener)
{
    ClientProxy* client = listener->getNextClient();
    if (client != nullptr) {
        m_server->adoptClient(client);
        updateStatus();
    }
}

void ServerApp::handle_clients_disconnected(const Event&)
{
    m_events->add_event(EventType::QUIT);
}

void
ServerApp::closeServer(Server* server)
{
    if (server == nullptr) {
        return;
    }

    // tell all clients to disconnect
    server->disconnect();

    // wait for clients to disconnect for up to timeout seconds
    double timeout = 3.0;
    EventQueueTimer* timer = m_events->newOneShotTimer(timeout, nullptr);
    m_events->add_handler(EventType::TIMER, timer,
                          [this](const auto& e){ handle_clients_disconnected(e); });
    m_events->add_handler(EventType::SERVER_DISCONNECTED, server,
                          [this](const auto& e){ handle_clients_disconnected(e); });

    m_events->loop();

    m_events->remove_handler(EventType::TIMER, timer);
    m_events->deleteTimer(timer);
    m_events->remove_handler(EventType::SERVER_DISCONNECTED, server);

    // done with server
    delete server;
}

void
ServerApp::stopRetryTimer()
{
    if (m_timer != nullptr) {
        m_events->remove_handler(EventType::TIMER, m_timer);
        m_events->deleteTimer(m_timer);
        m_timer = nullptr;
    }
}

void
ServerApp::updateStatus()
{
    updateStatus("");
}

void ServerApp::updateStatus(const std::string& msg)
{
    if (m_taskBarReceiver)
    {
        m_taskBarReceiver->updateStatus(m_server, msg);
    }
}

void
ServerApp::closeClientListener(ClientListener* listen)
{
    if (listen != nullptr) {
        m_events->remove_handler(EventType::CLIENT_LISTENER_CONNECTED, listen);
        delete listen;
    }
}

void
ServerApp::stopServer()
{
    if (m_serverState == kStarted) {
        closeServer(m_server);
        closeClientListener(m_listener);
        m_server = nullptr;
        m_listener = nullptr;
        m_serverState = kInitialized;
    }
    else if (m_serverState == kStarting) {
        stopRetryTimer();
        m_serverState = kInitialized;
    }
    assert(m_server == nullptr);
    assert(m_listener == nullptr);
}

void
ServerApp::closePrimaryClient(PrimaryClient* primaryClient)
{
    delete primaryClient;
}

void
ServerApp::closeServerScreen(inputleap::Screen* screen)
{
    if (screen != nullptr) {
        m_events->remove_handler(EventType::SCREEN_ERROR, screen->get_event_target());
        m_events->remove_handler(EventType::SCREEN_SUSPEND, screen->get_event_target());
        m_events->remove_handler(EventType::SCREEN_RESUME, screen->get_event_target());
        delete screen;
    }
}

void ServerApp::cleanupServer()
{
    stopServer();
    if (m_serverState == kInitialized) {
        closePrimaryClient(m_primaryClient);
        closeServerScreen(m_serverScreen);
        m_primaryClient = nullptr;
        m_serverScreen = nullptr;
        m_serverState   = kUninitialized;
    }
    else if (m_serverState == kInitializing ||
        m_serverState == kInitializingToStart) {
            stopRetryTimer();
            m_serverState = kUninitialized;
    }
    assert(m_primaryClient == nullptr);
    assert(m_serverScreen == nullptr);
    assert(m_serverState == kUninitialized);
}

void ServerApp::handle_retry()
{
    // discard old timer
    assert(m_timer != nullptr);
    stopRetryTimer();

    // try initializing/starting the server again
    switch (m_serverState) {
    case kUninitialized:
    case kInitialized:
    case kStarted:
        assert(0 && "bad internal server state");
        break;

    case kInitializing:
        LOG((CLOG_DEBUG1 "retry server initialization"));
        m_serverState = kUninitialized;
        if (!initServer()) {
            m_events->add_event(EventType::QUIT);
        }
        break;

    case kInitializingToStart:
        LOG((CLOG_DEBUG1 "retry server initialization"));
        m_serverState = kUninitialized;
        if (!initServer()) {
            m_events->add_event(EventType::QUIT);
        }
        else if (m_serverState == kInitialized) {
            LOG((CLOG_DEBUG1 "starting server"));
            if (!startServer()) {
                m_events->add_event(EventType::QUIT);
            }
        }
        break;

    case kStarting:
        LOG((CLOG_DEBUG1 "retry starting server"));
        m_serverState = kInitialized;
        if (!startServer()) {
            m_events->add_event(EventType::QUIT);
        }
        break;
    default:
        break;
    }
}

bool ServerApp::initServer()
{
    // skip if already initialized or initializing
    if (m_serverState != kUninitialized) {
        return true;
    }

    double retryTime;
    inputleap::Screen* serverScreen = nullptr;
    PrimaryClient* primaryClient = nullptr;
    try {
        std::string name = args().m_config->getCanonicalName(args().m_name);
        serverScreen    = openServerScreen();
        primaryClient   = openPrimaryClient(name, serverScreen);
        m_serverScreen  = serverScreen;
        m_primaryClient = primaryClient;
        m_serverState   = kInitialized;
        updateStatus();
        return true;
    }
    catch (XScreenUnavailable& e) {
        LOG((CLOG_WARN "primary screen unavailable: %s", e.what()));
        closePrimaryClient(primaryClient);
        closeServerScreen(serverScreen);
        updateStatus(std::string("primary screen unavailable: ") + e.what());
        retryTime = e.getRetryTime();
    }
    catch (XScreenOpenFailure& e) {
        LOG((CLOG_CRIT "failed to start server: %s", e.what()));
        closePrimaryClient(primaryClient);
        closeServerScreen(serverScreen);
        return false;
    }
    catch (XBase& e) {
        LOG((CLOG_CRIT "failed to start server: %s", e.what()));
        closePrimaryClient(primaryClient);
        closeServerScreen(serverScreen);
        return false;
    }

    if (args().m_restartable) {
        // install a timer and handler to retry later
        assert(m_timer == nullptr);
        LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
        m_timer = m_events->newOneShotTimer(retryTime, nullptr);
        m_events->add_handler(EventType::TIMER, m_timer,
                              [this](const auto& e){ handle_retry(); });
        m_serverState = kInitializing;
        return true;
    }
    else {
        // don't try again
        return false;
    }
}

inputleap::Screen*
ServerApp::openServerScreen()
{
    inputleap::Screen* screen = createScreen();
    if (!argsBase().m_dropTarget.empty()) {
        screen->setDropTarget(argsBase().m_dropTarget);
    }
    screen->setEnableDragDrop(argsBase().m_enableDragDrop);
    m_events->add_handler(EventType::SCREEN_ERROR, screen->get_event_target(),
                          [this](const auto& e){ handle_screen_error(); });
    m_events->add_handler(EventType::SCREEN_SUSPEND, screen->get_event_target(),
                          [this](const auto& e){ handle_suspend(); });
    m_events->add_handler(EventType::SCREEN_RESUME, screen->get_event_target(),
                          [this](const auto& e){ handle_resume(); });
    return screen;
}

static const char* family_string(IArchNetwork::EAddressFamily family)
{
    if (family == IArchNetwork::kINET)
        return "IPv4";
    if (family == IArchNetwork::kINET6)
        // assume IPv6 sockets are setup to support IPv4 traffic as well
        return "IPv4/IPv6";
    return "Unknown";
}

bool
ServerApp::startServer()
{
    // skip if already started or starting
    if (m_serverState == kStarting || m_serverState == kStarted) {
        return true;
    }

    // initialize if necessary
    if (m_serverState != kInitialized) {
        if (!initServer()) {
            // hard initialization failure
            return false;
        }
        if (m_serverState == kInitializing) {
            // not ready to start
            m_serverState = kInitializingToStart;
            return true;
        }
        assert(m_serverState == kInitialized);
    }

    double retryTime;
    ClientListener* listener = nullptr;
    try {
        auto listenAddress = args().m_config->get_listen_address();
        auto family = family_string(ARCH->getAddrFamily(listenAddress.getAddress()));
        listener   = openClientListener(listenAddress);
        m_server   = openServer(*args().m_config, m_primaryClient);
        listener->setServer(m_server);
        m_server->setListener(listener);
        m_listener = listener;
        updateStatus();

        // using CLOG_PRINT here allows the GUI to see that the server is started
        // regardless of which log level is set
        LOG((CLOG_PRINT "started server (%s), waiting for clients", family));
        m_serverState = kStarted;
        return true;
    }
    catch (XSocketAddressInUse& e) {
        LOG((CLOG_ERR "cannot listen for clients: %s", e.what()));
        closeClientListener(listener);
        updateStatus(std::string("cannot listen for clients: ") + e.what());
        retryTime = 1.0;
    }
    catch (XBase& e) {
        LOG((CLOG_CRIT "failed to start server: %s", e.what()));
        closeClientListener(listener);
        return false;
    }

    if (args().m_restartable) {
        // install a timer and handler to retry later
        assert(m_timer == nullptr);
        LOG((CLOG_DEBUG "retry in %.0f seconds", retryTime));
        m_timer = m_events->newOneShotTimer(retryTime, nullptr);
        m_events->add_handler(EventType::TIMER, m_timer,
                              [this](const auto& e){ handle_retry(); });
        m_serverState = kStarting;
        return true;
    }
    else {
        // don't try again
        return false;
    }
}

inputleap::Screen*
ServerApp::createScreen()
{
#if WINAPI_MSWINDOWS
    return new inputleap::Screen(new MSWindowsScreen(
        true, args().m_noHooks, args().m_stopOnDeskSwitch, m_events), m_events);
#endif
#if WINAPI_XWINDOWS
    return new inputleap::Screen(new XWindowsScreen(
        new XWindowsImpl(),
        args().m_display, true, 0, m_events), m_events);
#endif
#if WINAPI_CARBON
    return new inputleap::Screen(new OSXScreen(m_events, true), m_events);
#endif
    throw std::runtime_error("Failed to create screen, this shouldn't happen");
}

PrimaryClient* ServerApp::openPrimaryClient(const std::string& name, inputleap::Screen* screen)
{
    LOG((CLOG_DEBUG1 "creating primary screen"));
    return new PrimaryClient(name, screen);

}

void ServerApp::handle_screen_error()
{
    LOG((CLOG_CRIT "error on screen"));
    m_events->add_event(EventType::QUIT);
}

void ServerApp::handle_suspend()
{
    if (!m_suspended) {
        LOG((CLOG_INFO "suspend"));
        stopServer();
        m_suspended = true;
    }
}

void ServerApp::handle_resume()
{
    if (m_suspended) {
        LOG((CLOG_INFO "resume"));
        startServer();
        m_suspended = false;
    }
}

ClientListener*
ServerApp::openClientListener(const NetworkAddress& address)
{
    auto security_level = ConnectionSecurityLevel::PLAINTEXT;
    if (args().m_enableCrypto) {
        security_level = ConnectionSecurityLevel::ENCRYPTED;
        if (args().check_client_certificates) {
            security_level = ConnectionSecurityLevel::ENCRYPTED_AUTHENTICATED;
        }
    }

    ClientListener* listen = new ClientListener(
        address,
        std::make_unique<TCPSocketFactory>(m_events, getSocketMultiplexer()),
        m_events, security_level);

    m_events->add_handler(EventType::CLIENT_LISTENER_CONNECTED, listen,
                          [this, listen](const auto& e){ handle_client_connected(e, listen); });

    return listen;
}

Server*
ServerApp::openServer(Config& config, PrimaryClient* primaryClient)
{
    Server* server = new Server(config, primaryClient, m_serverScreen, m_events, args());
    try {
        m_events->add_handler(EventType::SERVER_DISCONNECTED, server,
                              [this](const auto& e){ handle_no_clients(); });
        m_events->add_handler(EventType::SERVER_SCREEN_SWITCHED, server,
                              [this](const auto& e){ handle_screen_switched(e); });

    } catch (std::bad_alloc &ba) {
        delete server;
        throw ba;
    }

    return server;
}

void ServerApp::handle_no_clients()
{
    updateStatus();
}

void ServerApp::handle_screen_switched(const Event& e)
{
    #ifdef WINAPI_XWINDOWS
        const auto& info = e.get_data_as<Server::SwitchToScreenInfo>();

        if (!args().m_screenChangeScript.empty()) {
            LOG((CLOG_INFO "Running shell script for screen \"%s\"", info.m_screen));

            signal(SIGCHLD, SIG_IGN);

            if (!access(args().m_screenChangeScript.c_str(), X_OK)) {
                pid_t pid = fork();
                if (pid == 0) {
                    execl(args().m_screenChangeScript.c_str(),args().m_screenChangeScript.c_str(),
                          info.m_screen,nullptr);
                    exit(0);
                } else if (pid < 0) {
                    LOG((CLOG_ERR "Script forking error"));
                    exit(1);
                }
            } else {
                LOG((CLOG_ERR "Script not accessible \"%s\"", args().m_screenChangeScript.c_str()));
            }
        }
    #endif
}

int
ServerApp::mainLoop()
{
    // create socket multiplexer.  this must happen after daemonization
    // on unix because threads evaporate across a fork().
    setSocketMultiplexer(std::make_unique<SocketMultiplexer>());

    // if configuration has no screens then add this system
    // as the default
    if (args().m_config->begin() == args().m_config->end()) {
        args().m_config->addScreen(args().m_name);
    }

    // set the contact address, if provided, in the config.
    // otherwise, if the config doesn't have an address, use
    // the default.
    if (listen_address_->isValid()) {
        args().m_config->set_listen_address(*listen_address_);
    }
    else if (!args().m_config->get_listen_address().isValid()) {
        args().m_config->set_listen_address(NetworkAddress(kDefaultPort));
    }

    // canonicalize the primary screen name
    std::string primaryName = args().m_config->getCanonicalName(args().m_name);
    if (primaryName.empty()) {
        LOG((CLOG_CRIT "unknown screen name `%s'", args().m_name.c_str()));
        return kExitFailed;
    }

    // start server, etc
    appUtil().startNode();

    // init ipc client after node start, since create a new screen wipes out
    // the event queue (the screen ctors call adoptBuffer).
    if (argsBase().m_enableIpc) {
        initIpcClient();
    }

    // handle hangup signal by reloading the server's configuration
    ARCH->setSignalHandler(Arch::kHANGUP, &reloadSignalHandler, nullptr);
    m_events->add_handler(EventType::SERVER_APP_RELOAD_CONFIG, m_events->getSystemTarget(),
                          [this](const auto& e){ reload_config(); });

    // handle force reconnect event by disconnecting clients.  they'll
    // reconnect automatically.
    m_events->add_handler(EventType::SERVER_APP_FORCE_RECONNECT, m_events->getSystemTarget(),
                          [this](const auto& e){ force_reconnect(); });

    // to work around the sticky meta keys problem, we'll give users
    // the option to reset the state of InputLeap server.
    m_events->add_handler(EventType::SERVER_APP_RESET_SERVER, m_events->getSystemTarget(),
                          [this](const auto& e){ reset_server(); });

    // run event loop.  if startServer() failed we're supposed to retry
    // later.  the timer installed by startServer() will take care of
    // that.
    DAEMON_RUNNING(true);

#if defined(MAC_OS_X_VERSION_10_7)

    Thread thread([this](){ run_events_loop(); });

    // wait until carbon loop is ready
    OSXScreen* screen = dynamic_cast<OSXScreen*>(
        m_serverScreen->getPlatformScreen());
    screen->waitForCarbonLoop();

    runCocoaApp();
#else
    m_events->loop();
#endif

    DAEMON_RUNNING(false);

    // close down
    LOG((CLOG_DEBUG1 "stopping server"));
    m_events->remove_handler(EventType::SERVER_APP_FORCE_RECONNECT, m_events->getSystemTarget());
    m_events->remove_handler(EventType::SERVER_APP_RELOAD_CONFIG, m_events->getSystemTarget());
    cleanupServer();
    updateStatus();
    LOG((CLOG_NOTE "stopped server"));

    if (argsBase().m_enableIpc) {
        cleanupIpcClient();
    }

    return kExitSuccess;
}

void ServerApp::reset_server()
{
    LOG((CLOG_DEBUG1 "resetting server"));
    stopServer();
    cleanupServer();
    startServer();
}

int
ServerApp::runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup)
{
    // general initialization
    listen_address_ = new NetworkAddress;
    args().m_config         = new Config(m_events);
    args().m_exename = ArgParser::parse_exename(argv[0]);

    // install caller's output filter
    if (outputter != nullptr) {
        CLOG->insert(outputter);
    }

    // run
    int result = startup(argc, argv);

    if (m_taskBarReceiver)
    {
        // done with task bar receiver
        delete m_taskBarReceiver;
    }

    delete args().m_config;
    delete listen_address_;
    return result;
}

int daemonMainLoopStatic(int argc, const char** argv) {
    return ServerApp::instance().daemonMainLoop(argc, argv);
}

int
ServerApp::standardStartup(int argc, char** argv)
{
    initApp(argc, argv);

    // daemonize if requested
    if (args().m_daemon) {
        return ARCH->daemonize(daemonName(), daemonMainLoopStatic);
    }
    else {
        return mainLoop();
    }
}

int
ServerApp::foregroundStartup(int argc, char** argv)
{
    initApp(argc, argv);

    // never daemonize
    return mainLoop();
}

const char*
ServerApp::daemonName() const
{
#if SYSAPI_WIN32
    return "InputLeap Server";
#elif SYSAPI_UNIX
    return "input-leaps";
#endif
}

const char*
ServerApp::daemonInfo() const
{
#if SYSAPI_WIN32
    return "Shares this computers mouse and keyboard with other computers.";
#elif SYSAPI_UNIX
    return "";
#endif
}

void
ServerApp::startNode()
{
    // start the server.  if this return false then we've failed and
    // we shouldn't retry.
    LOG((CLOG_DEBUG1 "starting server"));
    if (!startServer()) {
        m_bye(kExitFailed);
    }
}

} // namespace inputleap
