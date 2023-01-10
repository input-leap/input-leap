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

#include "ipc/IpcServer.h"

#include "ipc/Ipc.h"
#include "ipc/IpcClientProxy.h"
#include "ipc/IpcMessage.h"
#include "net/IDataSocket.h"
#include "io/IStream.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"
#include "base/Event.h"
#include "base/Log.h"

namespace inputleap {

IpcServer::IpcServer(IEventQueue* events, SocketMultiplexer* socketMultiplexer) :
    m_mock(false),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer),
    m_address(NetworkAddress(IPC_HOST, IPC_PORT))
{
    init();
}

IpcServer::IpcServer(IEventQueue* events, SocketMultiplexer* socketMultiplexer, int port) :
    m_mock(false),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer),
    m_address(NetworkAddress(IPC_HOST, port))
{
    init();
}

void
IpcServer::init()
{
    socket_ = std::make_unique<TCPListenSocket>(m_events, m_socketMultiplexer, IArchNetwork::kINET);

    m_address.resolve();

    m_events->adoptHandler(EventType::LISTEN_SOCKET_CONNECTING, socket_.get(),
        new TMethodEventJob<IpcServer>(
        this, &IpcServer::handleClientConnecting));
}

IpcServer::~IpcServer()
{
    if (m_mock) {
        return;
    }

    m_events->removeHandler(EventType::LISTEN_SOCKET_CONNECTING, socket_.get());
    socket_.reset();

    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        ClientList::iterator it;
        for (it = m_clients.begin(); it != m_clients.end(); it++) {
            deleteClient(*it);
        }
        m_clients.clear();
    }
}

void
IpcServer::listen()
{
    socket_->bind(m_address);
}

void
IpcServer::handleClientConnecting(const Event&, void*)
{
    auto stream = socket_->accept();
    if (!stream) {
        return;
    }

    LOG((CLOG_DEBUG "accepted ipc client connection"));

    IpcClientProxy* proxy = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_clientsMutex);
        proxy = new IpcClientProxy(std::move(stream), m_events);
        m_clients.push_back(proxy);
    }

    m_events->adoptHandler(EventType::IPC_CLIENT_PROXY_DISCONNECTED, proxy,
        new TMethodEventJob<IpcServer>(
        this, &IpcServer::handleClientDisconnected));

    m_events->adoptHandler(EventType::IPC_CLIENT_PROXY_MESSAGE_RECEIVED, proxy,
        new TMethodEventJob<IpcServer>(
        this, &IpcServer::handleMessageReceived));

    m_events->add_event(EventType::IPC_SERVER_CLIENT_CONNECTED, this,
                        create_event_data<IpcClientProxy*>(proxy));
}

void
IpcServer::handleClientDisconnected(const Event& e, void*)
{
    IpcClientProxy* proxy = static_cast<IpcClientProxy*>(e.getTarget());

    std::lock_guard<std::mutex> lock(m_clientsMutex);
    m_clients.remove(proxy);
    deleteClient(proxy);

    LOG((CLOG_DEBUG "ipc client proxy removed, connected=%d", m_clients.size()));
}

void
IpcServer::handleMessageReceived(const Event& e, void*)
{
    Event event(EventType::IPC_SERVER_MESSAGE_RECEIVED, this);
    event.setDataObject(e.getDataObject());
    m_events->add_event(std::move(event));
}

void
IpcServer::deleteClient(IpcClientProxy* proxy)
{
    m_events->removeHandler(EventType::IPC_CLIENT_PROXY_MESSAGE_RECEIVED, proxy);
    m_events->removeHandler(EventType::IPC_CLIENT_PROXY_DISCONNECTED, proxy);
    delete proxy;
}

bool
IpcServer::hasClients(EIpcClientType clientType) const
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    if (m_clients.empty()) {
        return false;
    }

    ClientList::const_iterator it;
    for (it = m_clients.begin(); it != m_clients.end(); it++) {
        // at least one client is alive and type matches, there are clients.
        IpcClientProxy* p = *it;
        if (!p->m_disconnecting && p->m_clientType == clientType) {
            return true;
        }
    }

    // all clients must be disconnecting, no active clients.
    return false;
}

void
IpcServer::send(const IpcMessage& message, EIpcClientType filterType)
{
    std::lock_guard<std::mutex> lock(m_clientsMutex);

    ClientList::iterator it;
    for (it = m_clients.begin(); it != m_clients.end(); it++) {
        IpcClientProxy* proxy = *it;
        if (proxy->m_clientType == filterType) {
            proxy->send(message);
        }
    }
}

} // namespace inputleap
