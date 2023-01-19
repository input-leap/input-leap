/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INPUTLEAP_LIB_NET_FWD_H
#define INPUTLEAP_LIB_NET_FWD_H

namespace inputleap {

// FingerprintData.h
struct FingerprintData;

// FingerprintDatabase.h
class FingerprintDatabase;

// IDataSocket.h
class IDataSocket;

// IListenSocket.h
class IListenSocket;

// ISocket.h
class ISocket;

// ISocketFactory.h
class ISocketFactory;

// ISocketMultiplexerJob.h
struct MultiplexerJobStatus;
class ISocketMultiplexerJob;

// NetworkAddress.h
class NetworkAddress;

// SecureListenSocket.h
class SecureListenSocket;

// SecureSocket.h
class SecureSocket;

// SocketMultiplexer.h
class SocketMultiplexer;

// TCPSocket.h
class TCPSocket;

// TCPSocketFactory.h
class TCPSocketFactory;

} // namespace inputleap

#endif // INPUTLEAP_LIB_NET_FWD_H
