/*
 * barrier -- mouse and keyboard sharing utility
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

IDataSocket*
SecureListenSocket::accept()
{
    SecureSocket* socket = NULL;
    try {
        socket = new SecureSocket(m_events, m_socketMultiplexer,
                                  ARCH->acceptSocket(m_socket, NULL), security_level_);
        socket->initSsl(true);

        if (socket != NULL) {
            setListeningJob();
        }

        bool loaded = socket->load_certificates(barrier::DataDirectories::ssl_certificate_path());
        if (!loaded) {
            delete socket;
            return NULL;
        }

        socket->secureAccept();

        return dynamic_cast<IDataSocket*>(socket);
    }
    catch (XArchNetwork&) {
        if (socket != NULL) {
            delete socket;
            setListeningJob();
        }
        return NULL;
    }
    catch (std::exception &ex) {
        if (socket != NULL) {
            delete socket;
            setListeningJob();
        }
        throw ex;
    }
}
