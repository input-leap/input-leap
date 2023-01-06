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

#pragma once

#include "net/ISocket.h"
#include "io/IStream.h"
#include "base/EventTypes.h"
#include <string>

//! Data stream socket interface
/*!
This interface defines the methods common to all network sockets that
represent a full-duplex data stream.
*/
class IDataSocket : public ISocket, public inputleap::IStream {
public:
    class ConnectionFailedInfo {
    public:
        ConnectionFailedInfo(const char* what) : m_what(what) { }
        std::string m_what;
    };

    IDataSocket(IEventQueue* events) { (void) events; }

    //! @name manipulators
    //@{

    //! Connect socket
    /*!
    Attempt to connect to a remote endpoint.  This returns immediately
    and sends a connected event when successful or a connection failed
    event when it fails.  The stream acts as if shutdown for input and
    output until the stream connects.
    */
    virtual void connect(const NetworkAddress&) = 0;

    //@}

    // ISocket overrides
    // close() and getEventTarget() aren't pure to work around a bug
    // in VC++6.  it claims the methods are unused locals and warns
    // that it's removing them.  it's presumably tickled by inheriting
    // methods with identical signatures from both superclasses.
    void close() override;
    void* getEventTarget() const override;

    // IStream overrides
    virtual bool isFatal() const = 0;
};
