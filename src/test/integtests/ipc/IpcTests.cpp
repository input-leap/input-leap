/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

// TODO: fix, tests failing intermittently on mac.
#ifndef WINAPI_CARBON

#define INPUTLEAP_TEST_ENV

#include "test/global/TestEventQueue.h"
#include "ipc/IpcServer.h"
#include "ipc/IpcClient.h"
#include "ipc/IpcServerProxy.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcClientProxy.h"
#include "ipc/Ipc.h"
#include "net/SocketMultiplexer.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueue.h"

#include <gtest/gtest.h>

#define TEST_IPC_PORT 24802

namespace inputleap {

class IpcTests : public ::testing::Test
{
public:
    IpcTests();
    virtual ~IpcTests();

    void connectToServer_handle_message_received(const Event& e);
    void sendMessageToServer_serverHandleMessageReceived(const Event& e);
    void sendMessageToClient_server_handle_client_connected(const Event& e);
    void sendMessageToClient_client_handle_message_received(const Event& e);

public:
    SocketMultiplexer m_multiplexer;
    bool m_connectToServer_helloMessageReceived;
    bool m_connectToServer_hasClientNode;
    IpcServer* m_connectToServer_server;
    std::string m_sendMessageToServer_receivedString;
    std::string m_sendMessageToClient_receivedString;
    IpcClient* m_sendMessageToServer_client;
    IpcServer* m_sendMessageToClient_server;
    TestEventQueue m_events;

};

TEST_F(IpcTests, connectToServer)
{
    SocketMultiplexer socketMultiplexer;
    IpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen();
    m_connectToServer_server = &server;

    m_events.add_handler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server,
                         [this](const auto& e)
    {
        connectToServer_handle_message_received(e);
    });

    IpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect();

    m_events.initQuitTimeout(5);
    m_events.loop();
    m_events.removeHandler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server);
    m_events.cleanupQuitTimeout();

    EXPECT_EQ(true, m_connectToServer_helloMessageReceived);
    EXPECT_EQ(true, m_connectToServer_hasClientNode);
}

TEST_F(IpcTests, sendMessageToServer)
{
    SocketMultiplexer socketMultiplexer;
    IpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen();

    // event handler sends "test" command to server.
    m_events.add_handler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server,
                         [this](const auto& e)
    {
        sendMessageToServer_serverHandleMessageReceived(e);
    });

    IpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect();
    m_sendMessageToServer_client = &client;

    m_events.initQuitTimeout(5);
    m_events.loop();
    m_events.removeHandler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server);
    m_events.cleanupQuitTimeout();

    EXPECT_EQ("test", m_sendMessageToServer_receivedString);
}

TEST_F(IpcTests, sendMessageToClient)
{
    SocketMultiplexer socketMultiplexer;
    IpcServer server(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    server.listen();
    m_sendMessageToClient_server = &server;

    // event handler sends "test" log line to client.
    m_events.add_handler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server,
                         [this](const auto& e)
    {
        sendMessageToClient_server_handle_client_connected(e);
    });

    IpcClient client(&m_events, &socketMultiplexer, TEST_IPC_PORT);
    client.connect();

    m_events.add_handler(EventType::IPC_CLIENT_MESSAGE_RECEIVED, &client,
                         [this](const auto& e)
    {
        sendMessageToClient_client_handle_message_received(e);
    });

    m_events.initQuitTimeout(5);
    m_events.loop();
    m_events.removeHandler(EventType::IPC_SERVER_MESSAGE_RECEIVED, &server);
    m_events.removeHandler(EventType::IPC_CLIENT_MESSAGE_RECEIVED, &client);
    m_events.cleanupQuitTimeout();

    EXPECT_EQ("test", m_sendMessageToClient_receivedString);
}

IpcTests::IpcTests() :
m_connectToServer_helloMessageReceived(false),
m_connectToServer_hasClientNode(false),
m_connectToServer_server(nullptr),
m_sendMessageToServer_client(nullptr),
m_sendMessageToClient_server(nullptr)
{
}

IpcTests::~IpcTests()
{
}

void IpcTests::connectToServer_handle_message_received(const Event& e)
{
    const auto& m = e.get_data_as<IpcMessage>();
    if (m.type() == kIpcHello) {
        m_connectToServer_hasClientNode =
            m_connectToServer_server->hasClients(kIpcClientNode);
        m_connectToServer_helloMessageReceived = true;
        m_events.raiseQuitEvent();
    }
}

void IpcTests::sendMessageToServer_serverHandleMessageReceived(const Event& e)
{
    const auto& m = e.get_data_as<IpcMessage>();
    if (m.type() == kIpcHello) {
        LOG((CLOG_DEBUG "client said hello, sending test to server"));
        IpcCommandMessage cm("test", true);
        m_sendMessageToServer_client->send(cm);
    }
    else if (m.type() == kIpcCommand) {
        const auto& cm = static_cast<const IpcCommandMessage&>(m);
        LOG((CLOG_DEBUG "got ipc command message, %d", cm.command().c_str()));
        m_sendMessageToServer_receivedString = cm.command();
        m_events.raiseQuitEvent();
    }
}

void IpcTests::sendMessageToClient_server_handle_client_connected(const Event& e)
{
    const auto& m = e.get_data_as<IpcMessage>();
    if (m.type() == kIpcHello) {
        LOG((CLOG_DEBUG "client said hello, sending test to client"));
        IpcLogLineMessage msg("test");
        m_sendMessageToClient_server->send(msg, kIpcClientNode);
    }
}

void IpcTests::sendMessageToClient_client_handle_message_received(const Event& e)
{
    const auto& m = e.get_data_as<IpcMessage>();
    if (m.type() == kIpcLogLine) {
        const auto& llm = static_cast<const IpcLogLineMessage&>(m);
        LOG((CLOG_DEBUG "got ipc log message, %d", llm.logLine().c_str()));
        m_sendMessageToClient_receivedString = llm.logLine();
        m_events.raiseQuitEvent();
    }
}

} // namespace inputleap

#endif // WINAPI_CARBON
