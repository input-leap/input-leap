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

#include "net/TCPSocketFactory.h"
#include "net/TCPSocket.h"
#include "net/TCPListenSocket.h"
#include "net/SecureSocket.h"
#include "net/SecureListenSocket.h"
#include "arch/Arch.h"
#include "base/Log.h"

namespace inputleap {

TCPSocketFactory::TCPSocketFactory(IEventQueue* events, SocketMultiplexer* socketMultiplexer) :
    m_events(events),
    m_socketMultiplexer(socketMultiplexer)
{
    // do nothing
}

TCPSocketFactory::~TCPSocketFactory()
{
    // do nothing
}

std::unique_ptr<IDataSocket>
    TCPSocketFactory::create(IArchNetwork::EAddressFamily family,
                             ConnectionSecurityLevel security_level) const
{
    if (security_level != ConnectionSecurityLevel::PLAINTEXT) {
        auto secure_socket = std::make_unique<SecureSocket>(m_events, m_socketMultiplexer, family,
                                                            security_level);
        secure_socket->initSsl(false);
        return secure_socket;
    } else {
        return std::make_unique<TCPSocket>(m_events, m_socketMultiplexer, family);
    }
}

std::unique_ptr<IListenSocket>
    TCPSocketFactory::create_listen(IArchNetwork::EAddressFamily family,
                                    ConnectionSecurityLevel security_level) const
{
    if (security_level != ConnectionSecurityLevel::PLAINTEXT) {
        return std::make_unique<SecureListenSocket>(m_events, m_socketMultiplexer, family,
                                                    security_level);
    } else {
        return std::make_unique<TCPListenSocket>(m_events, m_socketMultiplexer, family);
    }
}

} // namespace inputleap
