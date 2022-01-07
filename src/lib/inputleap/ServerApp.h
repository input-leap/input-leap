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
#include "server/Config.h"
#include "net/NetworkAddress.h"
#include "arch/Arch.h"
#include "arch/IArchMultithread.h"
#include "inputleap/ArgsBase.h"
#include "base/EventTypes.h"

#include <map>

enum EServerState {
    kUninitialized,
    kInitializing,
    kInitializingToStart,
    kInitialized,
    kStarting,
    kStarted
};

class Server;
namespace inputleap { class Screen; }
class ClientListener;
class EventQueueTimer;
class ILogOutputter;
class IEventQueue;
class ServerArgs;

class ServerApp : public App {
public:
    ServerApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver);
    virtual ~ServerApp();

    // Parse server specific command line arguments.
    void parseArgs(int argc, const char* const* argv);

    // Prints help specific to server.
    void help();

    // Returns arguments that are common and for server.
    ServerArgs& args() const { return (ServerArgs&)argsBase(); }

    const char* daemonName() const;
    const char* daemonInfo() const;

    // TODO: Document these functions.
    static void reloadSignalHandler(Arch::ESignal, void*);

    void reloadConfig(const Event&, void*);
    void loadConfig();
    bool loadConfig(const std::string& pathname);
    void forceReconnect(const Event&, void*);
    void resetServer(const Event&, void*);
    void handleClientConnected(const Event&, void* vlistener);
    void handleClientsDisconnected(const Event&, void*);
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
    void retryHandler(const Event&, void*);
    inputleap::Screen* openServerScreen();
    inputleap::Screen* createScreen();
    PrimaryClient* openPrimaryClient(const std::string& name, inputleap::Screen* screen);
    void handleScreenError(const Event&, void*);
    void handleSuspend(const Event&, void*);
    void handleResume(const Event&, void*);
    ClientListener* openClientListener(const NetworkAddress& address);
    Server* openServer(Config& config, PrimaryClient* primaryClient);
    void handleNoClients(const Event&, void*);
    bool startServer();
    int mainLoop();
    int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup);
    int standardStartup(int argc, char** argv);
    int foregroundStartup(int argc, char** argv);
    void startNode();

    static ServerApp& instance() { return (ServerApp&)App::instance(); }

    Server* getServerPtr() { return m_server; }

    Server*                m_server;
    EServerState        m_serverState;
    inputleap::Screen* m_serverScreen;
    PrimaryClient*        m_primaryClient;
    ClientListener*        m_listener;
    EventQueueTimer*    m_timer;
    NetworkAddress*        m_barrierAddress;

private:
    void handleScreenSwitched(const Event&, void*  data);
};

// configuration file name
#if SYSAPI_WIN32
#define USR_CONFIG_NAME "barrier.sgc"
#define SYS_CONFIG_NAME "barrier.sgc"
#elif SYSAPI_UNIX
#define USR_CONFIG_NAME ".barrier.conf"
#define SYS_CONFIG_NAME "barrier.conf"
#endif
