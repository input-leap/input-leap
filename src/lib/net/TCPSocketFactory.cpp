/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#include "net/TCPSocketFactory.h"
#include "net/TCPSocket.h"
#include "net/TCPListenSocket.h"
#include "net/SecureSocket.h"
#include "net/SecureListenSocket.h"
#include "arch/Arch.h"
#include "base/Log.h"

//
// TCPSocketFactory
//

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

IDataSocket* TCPSocketFactory::create(IArchNetwork::EAddressFamily family,
                                      ConnectionSecurityLevel security_level) const
{
    if (security_level != ConnectionSecurityLevel::PLAINTEXT) {
        SecureSocket* secureSocket = new SecureSocket(m_events, m_socketMultiplexer, family,
                                                      security_level);
        secureSocket->initSsl (false);
        return secureSocket;
    }
    else {
        return new TCPSocket(m_events, m_socketMultiplexer, family);
    }
}

IListenSocket* TCPSocketFactory::createListen(IArchNetwork::EAddressFamily family,
                                              ConnectionSecurityLevel security_level) const
{
    IListenSocket* socket = nullptr;
    if (security_level != ConnectionSecurityLevel::PLAINTEXT) {
        socket = new SecureListenSocket(m_events, m_socketMultiplexer, family, security_level);
    }
    else {
        socket = new TCPListenSocket(m_events, m_socketMultiplexer, family);
    }

    return socket;
}
