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
#include "base/EventTypes.h"

class IDataSocket;

//! Listen socket interface
/*!
This interface defines the methods common to all network sockets that
listen for incoming connections.
*/
class IListenSocket : public ISocket {
public:
    //! @name manipulators
    //@{

    //! Accept connection
    /*!
    Accept a connection, returning a socket representing the full-duplex
    data stream.  Returns nullptr if no socket is waiting to be accepted.
    This is only valid after a call to \c bind().
    */
    virtual IDataSocket*
                        accept() = 0;

    //@}
};
