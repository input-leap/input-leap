/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#include "test/mock/server/MockConfig.h"
#include "test/mock/server/MockPrimaryClient.h"
#include "test/mock/inputleap/MockScreen.h"
#include "test/mock/server/MockInputFilter.h"
#include "test/global/TestEventQueue.h"
#include "server/Server.h"
#include "server/ClientListener.h"
#include "server/ClientProxy.h"
#include "client/Client.h"
#include "inputleap/FileChunk.h"
#include "inputleap/StreamChunker.h"
#include "net/SocketMultiplexer.h"
#include "net/NetworkAddress.h"
#include "net/TCPSocketFactory.h"
#include "mt/Thread.h"
#include "base/Log.h"
#include <stdexcept>

#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdio.h>

namespace inputleap {

using namespace std;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::Invoke;

#define TEST_PORT 24803
#define TEST_HOST "localhost"

const size_t kMockDataSize = 1024 * 1024 * 10; // 10MB
const std::uint16_t kMockDataChunkIncrement = 1024; // 1KB
const char* kMockFilename = "NetworkTests.mock";
const size_t kMockFileSize = 1024 * 1024 * 10; // 10MB

void getScreenShape(std::int32_t& x, std::int32_t& y, std::int32_t& w, std::int32_t& h);
void getCursorPos(std::int32_t& x, std::int32_t& y);
std::uint8_t* newMockData(size_t size);
void createFile(fstream& file, const char* filename, size_t size);

class NetworkTests : public ::testing::Test
{
public:
    NetworkTests() :
        m_mockData(nullptr),
        m_mockDataSize(0),
        m_mockFileSize(0)
    {
        m_mockData = newMockData(kMockDataSize);
        createFile(m_mockFile, kMockFilename, kMockFileSize);
    }

    ~NetworkTests()
    {
        remove(kMockFilename);
        delete[] m_mockData;
    }

    void                sendMockData(void* eventTarget);

    void sendToClient_mockData_handle_client_connected(const Event&, ClientListener* listener);
    void sendToClient_mockData_file_receive_completed(const Event&);

    void sendToClient_mockFile_handle_client_connected(const Event&, ClientListener* listener);
    void sendToClient_mockFile_file_receive_completed(const Event& event);

    void sendToServer_mockData_handle_client_connected(const Event&, Client* client);
    void sendToServer_mockData_file_receive_completed(const Event& event);

    void sendToServer_mockFile_handle_client_connected(const Event&, Client* client);
    void sendToServer_mockFile_file_recieve_completed(const Event& event);

public:
    TestEventQueue        m_events;
    std::uint8_t* m_mockData;
    size_t                m_mockDataSize;
    fstream                m_mockFile;
    size_t                m_mockFileSize;
};

TEST_F(NetworkTests, sendToClient_mockData)
{
    // server and client
    NetworkAddress serverAddress(TEST_HOST, TEST_PORT);

    serverAddress.resolve();

    // server
    SocketMultiplexer serverSocketMultiplexer;
    TCPSocketFactory* serverSocketFactory = new TCPSocketFactory(&m_events, &serverSocketMultiplexer);
    ClientListener listener(serverAddress, serverSocketFactory, &m_events,
                            ConnectionSecurityLevel::PLAINTEXT);
    NiceMock<MockScreen> serverScreen;
    NiceMock<MockPrimaryClient> primaryClient;
    NiceMock<MockConfig> serverConfig;
    NiceMock<MockInputFilter> serverInputFilter;

    m_events.add_handler(EventType::CLIENT_LISTENER_CONNECTED, &listener,
                         [this, &listener](const auto& e)
    {
        sendToClient_mockData_handle_client_connected(e, &listener);
    });

    ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
    ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));

    ServerArgs serverArgs;
    serverArgs.m_enableDragDrop = true;
    Server server(serverConfig, &primaryClient, &serverScreen, &m_events, serverArgs);
    server.m_mock = true;
    listener.setServer(&server);

    // client
    NiceMock<MockScreen> clientScreen;
    SocketMultiplexer clientSocketMultiplexer;
    TCPSocketFactory* clientSocketFactory = new TCPSocketFactory(&m_events, &clientSocketMultiplexer);

    ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
    ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));


    ClientArgs clientArgs;
    clientArgs.m_enableDragDrop = true;
    clientArgs.m_enableCrypto = false;
    Client client(&m_events, "stub", serverAddress, clientSocketFactory, &clientScreen, clientArgs);

    m_events.add_handler(EventType::FILE_RECEIVE_COMPLETED, &client,
                         [this](const auto& e)
    {
        sendToClient_mockData_file_receive_completed(e);
    });

    client.connect();

    m_events.initQuitTimeout(10);
    m_events.loop();
    m_events.removeHandler(EventType::CLIENT_LISTENER_CONNECTED, &listener);
    m_events.removeHandler(EventType::FILE_RECEIVE_COMPLETED, &client);
    m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToClient_mockFile)
{
    // server and client
    NetworkAddress serverAddress(TEST_HOST, TEST_PORT);

    serverAddress.resolve();

    // server
    SocketMultiplexer serverSocketMultiplexer;
    TCPSocketFactory* serverSocketFactory = new TCPSocketFactory(&m_events, &serverSocketMultiplexer);
    ClientListener listener(serverAddress, serverSocketFactory, &m_events,
                            ConnectionSecurityLevel::PLAINTEXT);
    NiceMock<MockScreen> serverScreen;
    NiceMock<MockPrimaryClient> primaryClient;
    NiceMock<MockConfig> serverConfig;
    NiceMock<MockInputFilter> serverInputFilter;

    m_events.add_handler(EventType::CLIENT_LISTENER_CONNECTED, &listener,
                         [this, &listener](const auto& e)
    {
        sendToClient_mockFile_handle_client_connected(e, &listener);
    });

    ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
    ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));

    ServerArgs serverArgs;
    serverArgs.m_enableDragDrop = true;
    Server server(serverConfig, &primaryClient, &serverScreen, &m_events, serverArgs);
    server.m_mock = true;
    listener.setServer(&server);

    // client
    NiceMock<MockScreen> clientScreen;
    SocketMultiplexer clientSocketMultiplexer;
    TCPSocketFactory* clientSocketFactory = new TCPSocketFactory(&m_events, &clientSocketMultiplexer);

    ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
    ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));


    ClientArgs clientArgs;
    clientArgs.m_enableDragDrop = true;
    clientArgs.m_enableCrypto = false;
    Client client(&m_events, "stub", serverAddress, clientSocketFactory, &clientScreen, clientArgs);

    m_events.add_handler(EventType::FILE_RECEIVE_COMPLETED, &client,
                         [this](const auto& e)
    {
        sendToClient_mockFile_file_receive_completed(e);
    });

    client.connect();

    m_events.initQuitTimeout(10);
    m_events.loop();
    m_events.removeHandler(EventType::CLIENT_LISTENER_CONNECTED, &listener);
    m_events.removeHandler(EventType::FILE_RECEIVE_COMPLETED, &client);
    m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToServer_mockData)
{
    // server and client
    NetworkAddress serverAddress(TEST_HOST, TEST_PORT);
    serverAddress.resolve();

    // server
    SocketMultiplexer serverSocketMultiplexer;
    TCPSocketFactory* serverSocketFactory = new TCPSocketFactory(&m_events, &serverSocketMultiplexer);
    ClientListener listener(serverAddress, serverSocketFactory, &m_events,
                            ConnectionSecurityLevel::PLAINTEXT);
    NiceMock<MockScreen> serverScreen;
    NiceMock<MockPrimaryClient> primaryClient;
    NiceMock<MockConfig> serverConfig;
    NiceMock<MockInputFilter> serverInputFilter;

    ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
    ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));

    ServerArgs serverArgs;
    serverArgs.m_enableDragDrop = true;
    Server server(serverConfig, &primaryClient, &serverScreen, &m_events, serverArgs);
    server.m_mock = true;
    listener.setServer(&server);

    // client
    NiceMock<MockScreen> clientScreen;
    SocketMultiplexer clientSocketMultiplexer;
    TCPSocketFactory* clientSocketFactory = new TCPSocketFactory(&m_events, &clientSocketMultiplexer);

    ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
    ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

    ClientArgs clientArgs;
    clientArgs.m_enableDragDrop = true;
    clientArgs.m_enableCrypto = false;
    Client client(&m_events, "stub", serverAddress, clientSocketFactory, &clientScreen, clientArgs);

    m_events.add_handler(EventType::CLIENT_LISTENER_CONNECTED, &listener,
                         [this, &client](const auto& e)
    {
        sendToServer_mockData_handle_client_connected(e, &client);
    });

    m_events.add_handler(EventType::FILE_RECEIVE_COMPLETED, &server,
                         [this](const auto& e)
    {
        sendToServer_mockData_file_receive_completed(e);
    });

    client.connect();

    m_events.initQuitTimeout(10);
    m_events.loop();
    m_events.removeHandler(EventType::CLIENT_LISTENER_CONNECTED, &listener);
    m_events.removeHandler(EventType::FILE_RECEIVE_COMPLETED, &server);
    m_events.cleanupQuitTimeout();
}

TEST_F(NetworkTests, sendToServer_mockFile)
{
    // server and client
    NetworkAddress serverAddress(TEST_HOST, TEST_PORT);

    serverAddress.resolve();

    // server
    SocketMultiplexer serverSocketMultiplexer;
    TCPSocketFactory* serverSocketFactory = new TCPSocketFactory(&m_events, &serverSocketMultiplexer);
    ClientListener listener(serverAddress, serverSocketFactory, &m_events,
                            ConnectionSecurityLevel::PLAINTEXT);
    NiceMock<MockScreen> serverScreen;
    NiceMock<MockPrimaryClient> primaryClient;
    NiceMock<MockConfig> serverConfig;
    NiceMock<MockInputFilter> serverInputFilter;

    ON_CALL(serverConfig, isScreen(_)).WillByDefault(Return(true));
    ON_CALL(serverConfig, getInputFilter()).WillByDefault(Return(&serverInputFilter));

    ServerArgs serverArgs;
    serverArgs.m_enableDragDrop = true;
    Server server(serverConfig, &primaryClient, &serverScreen, &m_events, serverArgs);
    server.m_mock = true;
    listener.setServer(&server);

    // client
    NiceMock<MockScreen> clientScreen;
    SocketMultiplexer clientSocketMultiplexer;
    TCPSocketFactory* clientSocketFactory = new TCPSocketFactory(&m_events, &clientSocketMultiplexer);

    ON_CALL(clientScreen, getShape(_, _, _, _)).WillByDefault(Invoke(getScreenShape));
    ON_CALL(clientScreen, getCursorPos(_, _)).WillByDefault(Invoke(getCursorPos));

    ClientArgs clientArgs;
    clientArgs.m_enableDragDrop = true;
    clientArgs.m_enableCrypto = false;
    Client client(&m_events, "stub", serverAddress, clientSocketFactory, &clientScreen, clientArgs);

    m_events.add_handler(EventType::CLIENT_LISTENER_CONNECTED, &listener,
                         [this, &client](const auto& e)
    {
        sendToServer_mockFile_handle_client_connected(e, &client);
    });

    m_events.add_handler(EventType::FILE_RECEIVE_COMPLETED, &server,
                         [this](const auto& e)
    {
        sendToServer_mockFile_file_recieve_completed(e);
    });

    client.connect();

    m_events.initQuitTimeout(10);
    m_events.loop();
    m_events.removeHandler(EventType::CLIENT_LISTENER_CONNECTED, &listener);
    m_events.removeHandler(EventType::FILE_RECEIVE_COMPLETED, &server);
    m_events.cleanupQuitTimeout();
}

void NetworkTests::sendToClient_mockData_handle_client_connected(const Event&,
                                                                 ClientListener* listener)
{
    Server* server = listener->getServer();

    ClientProxy* client = listener->getNextClient();
    if (client == nullptr) {
        throw runtime_error("client is null");
    }

    BaseClientProxy* bcp = client;
    server->adoptClient(bcp);
    server->setActive(bcp);

    sendMockData(server);
}

void NetworkTests::sendToClient_mockData_file_receive_completed(const Event& event)
{
    Client* client = static_cast<Client*>(event.getTarget());
    EXPECT_TRUE(client->isReceivedFileSizeValid());

    m_events.raiseQuitEvent();
}

void NetworkTests::sendToClient_mockFile_handle_client_connected(const Event&,
                                                                 ClientListener* listener)
{
    Server* server = listener->getServer();

    ClientProxy* client = listener->getNextClient();
    if (client == nullptr) {
        throw runtime_error("client is null");
    }

    BaseClientProxy* bcp = client;
    server->adoptClient(bcp);
    server->setActive(bcp);

    server->sendFileToClient(kMockFilename);
}

void NetworkTests::sendToClient_mockFile_file_receive_completed(const Event& event)
{
    Client* client = static_cast<Client*>(event.getTarget());
    EXPECT_TRUE(client->isReceivedFileSizeValid());

    m_events.raiseQuitEvent();
}

void NetworkTests::sendToServer_mockData_handle_client_connected(const Event&, Client* client)
{
    sendMockData(client);
}

void NetworkTests::sendToServer_mockData_file_receive_completed(const Event& event)
{
    Server* server = static_cast<Server*>(event.getTarget());
    EXPECT_TRUE(server->isReceivedFileSizeValid());

    m_events.raiseQuitEvent();
}

void NetworkTests::sendToServer_mockFile_handle_client_connected(const Event&, Client* client)
{
    client->sendFileToServer(kMockFilename);
}

void NetworkTests::sendToServer_mockFile_file_recieve_completed(const Event& event)
{
    Server* server = static_cast<Server*>(event.getTarget());
    EXPECT_TRUE(server->isReceivedFileSizeValid());

    m_events.raiseQuitEvent();
}

void
NetworkTests::sendMockData(void* eventTarget)
{
    // send first message (file size)
    std::string size = inputleap::string::sizeTypeToString(kMockDataSize);
    FileChunk* sizeMessage = FileChunk::start(size);

    m_events.add_event(EventType::FILE_CHUNK_SENDING, eventTarget,
                       create_event_data<FileChunk>(*sizeMessage));
    delete sizeMessage;

    // send chunk messages with incrementing chunk size
    size_t lastSize = 0;
    size_t sentLength = 0;
    while (true) {
        size_t dataSize = lastSize + kMockDataChunkIncrement;

        // make sure we don't read too much from the mock data.
        if (sentLength + dataSize > kMockDataSize) {
            dataSize = kMockDataSize - sentLength;
        }

        // first byte is the chunk mark, last is \0
        FileChunk* chunk = FileChunk::data(m_mockData, dataSize);
        m_events.add_event(EventType::FILE_CHUNK_SENDING, eventTarget,
                           create_event_data<FileChunk>(*chunk));
        delete chunk;

        sentLength += dataSize;
        lastSize = dataSize;

        if (sentLength == kMockDataSize) {
            break;
        }

    }

    // send last message
    FileChunk* transferFinished = FileChunk::end();
    m_events.add_event(EventType::FILE_CHUNK_SENDING, eventTarget,
                       create_event_data<FileChunk>(*transferFinished));
    delete transferFinished;
}

std::uint8_t* newMockData(size_t size)
{
    std::uint8_t* buffer = new std::uint8_t[size];

    std::uint8_t* data = buffer;
    const std::uint8_t head[] = "mock head... ";
    size_t headSize = sizeof(head) - 1;
    const std::uint8_t tail[] = "... mock tail";
    size_t tailSize = sizeof(tail) - 1;
    const std::uint8_t barrierRocks[] = "barrier\0 rocks! ";
    size_t barrierRocksSize = sizeof(barrierRocks) - 1;

    memcpy(data, head, headSize);
    data += headSize;

    size_t times = (size - headSize - tailSize) / barrierRocksSize;
    for (size_t i = 0; i < times; ++i) {
        memcpy(data, barrierRocks, barrierRocksSize);
        data += barrierRocksSize;
    }

    size_t remainder = (size - headSize - tailSize) % barrierRocksSize;
    if (remainder != 0) {
        memset(data, '.', remainder);
        data += remainder;
    }

    memcpy(data, tail, tailSize);
    return buffer;
}

void
createFile(fstream& file, const char* filename, size_t size)
{
    std::uint8_t* buffer = newMockData(size);

    file.open(filename, ios::out | ios::binary);
    if (!file.is_open()) {
        throw runtime_error("file not open");
    }

    file.write(reinterpret_cast<char*>(buffer), size);
    file.close();

    delete[] buffer;
}

void getScreenShape(std::int32_t& x, std::int32_t& y, std::int32_t& w, std::int32_t& h)
{
    x = 0;
    y = 0;
    w = 1;
    h = 1;
}

void getCursorPos(std::int32_t& x, std::int32_t& y)
{
    x = 0;
    y = 0;
}

} // namespace inputleap

#endif // WINAPI_CARBON
