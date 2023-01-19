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

#pragma once

#include "inputleap/ArgsBase.h"
#include "inputleap/App.h"
#include "inputleap/Fwd.h"
#include "base/Fwd.h"
#include "server/Config.h"
#include "net/NetworkAddress.h"
#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "base/EventTypes.h"
#include "ServerArgs.h"

#include <map>

namespace inputleap {

enum EServerState {
    kUninitialized,
    kInitializing,
    kInitializingToStart,
    kInitialized,
    kStarting,
    kStarted
};

class Server;
class ClientListener;

class ServerApp : public App {
public:
    ServerApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver);
    ~ServerApp() override;

    // Parse server specific command line arguments.
    void parseArgs(int argc, const char* const* argv) override;

    // Prints help specific to server.
    void help() override;

    // Returns arguments that are common and for server.
    ServerArgs& args() const { return static_cast<ServerArgs&>(argsBase()); }

    const char* daemonName() const override;
    const char* daemonInfo() const override;

    // TODO: Document these functions.
    static void reloadSignalHandler(Arch::ESignal, void*);

    void reload_config();
    void loadConfig() override;
    bool loadConfig(const std::string& pathname) override;
    void force_reconnect();
    void reset_server();
    void handle_client_connected(const Event& event, ClientListener* listener);
    void handle_clients_disconnected(const Event&);
    void closeServer(Server* server);
    void stopRetryTimer();
    void updateStatus();
    void updateStatus(const std::string& msg);
    void closeClientListener(ClientListener* listen);
    void stopServer();
    void closePrimaryClient(PrimaryClient* primaryClient);
    void closeServerScreen(inputleap::Screen* screen);
    void cleanupServer();
    bool initServer();
    void handle_retry();
    inputleap::Screen* openServerScreen();
    inputleap::Screen* createScreen() override;
    PrimaryClient* openPrimaryClient(const std::string& name, inputleap::Screen* screen);
    void handle_screen_error();
    void handle_suspend();
    void handle_resume();
    ClientListener* openClientListener(const NetworkAddress& address);
    Server* openServer(Config& config, PrimaryClient* primaryClient);
    void handle_no_clients();
    bool startServer();
    int mainLoop() override;
    int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup) override;
    int standardStartup(int argc, char** argv) override;
    int foregroundStartup(int argc, char** argv) override;
    void startNode() override;

    static ServerApp& instance() { return static_cast<ServerApp&>(App::instance()); }

    Server* getServerPtr() { return m_server; }

    Server* m_server;
    EServerState m_serverState;
    inputleap::Screen* m_serverScreen;
    PrimaryClient* m_primaryClient;
    ClientListener* m_listener;
    EventQueueTimer* m_timer;
    NetworkAddress* listen_address_;

private:
    void handle_screen_switched(const Event& event);
};

// configuration file name
#if SYSAPI_WIN32
#define USR_CONFIG_NAME "input-leap.sgc"
#define SYS_CONFIG_NAME "input-leap.sgc"
#elif SYSAPI_UNIX
#define USR_CONFIG_NAME ".input-leap.conf"
#define SYS_CONFIG_NAME "input-leap.conf"
#endif

} // namespace inputleap
