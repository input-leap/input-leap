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

#include "Fwd.h"
#include "base/Fwd.h"
#include "net/Fwd.h"
#include "inputleap/App.h"
#include "ClientArgs.h"

namespace inputleap {

class Client;

class ClientApp : public App {
public:
    ClientApp(IEventQueue* events, CreateTaskBarReceiverFunc createTaskBarReceiver);
    ~ClientApp() override;

    // Parse client specific command line arguments.
    void parseArgs(int argc, const char* const* argv) override;

    // Prints help specific to client.
    void help() override;

    // Returns arguments that are common and for client.
    ClientArgs& args() const { return static_cast<ClientArgs&>(argsBase()); }

    const char* daemonName() const override;
    const char* daemonInfo() const override;

    // TODO: move to server only (not supported on client)
    void loadConfig() override { }
    bool loadConfig(const std::string& pathname) override { (void) pathname; return false; }

    int foregroundStartup(int argc, char** argv) override;
    int standardStartup(int argc, char** argv) override;
    int runInner(int argc, char** argv, ILogOutputter* outputter, StartupFunc startup) override;
    inputleap::Screen* createScreen() override;
    void updateStatus();
    void updateStatus(const std::string& msg);
    void resetRestartTimeout();
    double nextRestartTimeout();
    void handle_screen_error();
    inputleap::Screen* openClientScreen();
    void closeClientScreen(inputleap::Screen* screen);
    void handle_client_restart(const Event& event, EventQueueTimer* timer);
    void scheduleClientRestart(double retryTime);
    void handle_client_connected();
    void handle_client_failed(const Event& e);
    void handle_client_disconnected();
    Client* openClient(const std::string& name, const NetworkAddress& address,
                inputleap::Screen* screen);
    void closeClient(Client* client);
    bool startClient();
    void stopClient();
    int mainLoop() override;
    void startNode() override;

    static ClientApp& instance() { return static_cast<ClientApp&>(App::instance()); }

    Client* getClientPtr() { return m_client; }

private:
    Client* m_client;
    inputleap::Screen* m_clientScreen;
    NetworkAddress* m_serverAddress;
};

} // namespace inputleap
