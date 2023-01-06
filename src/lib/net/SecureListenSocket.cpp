/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#include "SecureListenSocket.h"

#include "SecureSocket.h"
#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "common/DataDirectories.h"
#include "base/String.h"

SecureListenSocket::SecureListenSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer,
                                       IArchNetwork::EAddressFamily family,
                                       ConnectionSecurityLevel security_level) :
    TCPListenSocket(events, socketMultiplexer, family),
    security_level_{security_level}
{
}

std::unique_ptr<IDataSocket> SecureListenSocket::accept()
{
    std::unique_ptr<SecureSocket> socket;
    try {
        socket = std::make_unique<SecureSocket>(m_events, m_socketMultiplexer,
                                                 ARCH->acceptSocket(m_socket, nullptr),
                                                security_level_);
        socket->initSsl(true);
        setListeningJob();

        bool loaded = socket->load_certificates(inputleap::DataDirectories::ssl_certificate_path());
        if (!loaded) {
            return nullptr;
        }

        socket->secureAccept();

        return socket;
    }
    catch (XArchNetwork&) {
        if (socket) {
            setListeningJob();
        }
        return nullptr;
    }
    catch (std::exception &ex) {
        if (socket) {
            setListeningJob();
        }
        throw ex;
    }
}
