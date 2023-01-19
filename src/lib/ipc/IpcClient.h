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

#pragma once

#include "net/NetworkAddress.h"
#include "net/TCPSocket.h"
#include "base/EventTypes.h"
#include <memory>

namespace inputleap {

class IpcServerProxy;
class IpcMessage;
class SocketMultiplexer;

//! IPC client for communication between daemon and GUI.
/*!
 * See \ref IpcServer description.
 */
class IpcClient {
public:
    IpcClient(IEventQueue* events, SocketMultiplexer* socketMultiplexer);
    IpcClient(IEventQueue* events, SocketMultiplexer* socketMultiplexer, int port);
    virtual ~IpcClient();

    //! @name manipulators
    //@{

    //! Connects to the IPC server at localhost.
    void connect();

    //! Disconnects from the IPC server.
    void disconnect();

    //! Sends a message to the server.
    void send(const IpcMessage& message);

    //@}

private:
    void init();
    void handle_connected();
    void handle_message_received(const Event& event);

private:
    NetworkAddress m_serverAddress;
    TCPSocket m_socket;
    std::unique_ptr<IpcServerProxy> server_;
    IEventQueue* m_events;
};

} // namespace inputleap
