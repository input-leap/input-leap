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

#include "ipc/IpcClient.h"
#include "ipc/Ipc.h"
#include "ipc/IpcServerProxy.h"
#include "ipc/IpcMessage.h"
#include <cassert>

namespace inputleap {

IpcClient::IpcClient(IEventQueue* events, SocketMultiplexer* socketMultiplexer) :
    m_serverAddress(NetworkAddress(IPC_HOST, IPC_PORT)),
    m_socket(events, socketMultiplexer, IArchNetwork::kINET),
    m_events(events)
{
    init();
}

IpcClient::IpcClient(IEventQueue* events, SocketMultiplexer* socketMultiplexer, int port) :
    m_serverAddress(NetworkAddress(IPC_HOST, port)),
    m_socket(events, socketMultiplexer, IArchNetwork::kINET),
    m_events(events)
{
    init();
}

void
IpcClient::init()
{
    m_serverAddress.resolve();
}

IpcClient::~IpcClient()
{
}

void
IpcClient::connect()
{
    m_events->add_handler(EventType::DATA_SOCKET_CONNECTED, m_socket.get_event_target(),
                          [this](const auto& e){ handle_connected(); });

    m_socket.connect(m_serverAddress);
    server_ = std::make_unique<IpcServerProxy>(m_socket, m_events);

    m_events->add_handler(EventType::IPC_SERVER_PROXY_MESSAGE_RECEIVED, server_.get(),
                          [this](const auto& e){ handle_message_received(e); });
}

void
IpcClient::disconnect()
{
    m_events->remove_handler(EventType::DATA_SOCKET_CONNECTED, m_socket.get_event_target());
    m_events->remove_handler(EventType::IPC_SERVER_PROXY_MESSAGE_RECEIVED, server_.get());

    server_->disconnect();
    server_.reset();
}

void
IpcClient::send(const IpcMessage& message)
{
    assert(server_);
    server_->send(message);
}

void IpcClient::handle_connected()
{
    m_events->add_event(EventType::IPC_CLIENT_CONNECTED, this);

    IpcHelloMessage message(kIpcClientNode);
    send(message);
}

void IpcClient::handle_message_received(const Event& e)
{
    Event event(EventType::IPC_CLIENT_MESSAGE_RECEIVED, this);
    event.clone_data_from(e);
    m_events->add_event(std::move(event));
}

} // namespace inputleap
