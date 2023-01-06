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

#include "io/XIO.h"
#include "base/XBase.h"

#include <cstdint>

//! Generic socket exception
XBASE_SUBCLASS(XSocket, XBase);

//! Socket bad address exception
/*!
Thrown when attempting to create an invalid network address.
*/
class XSocketAddress : public XSocket {
public:
    //! Failure codes
    enum EError {
        kUnknown,        //!< Unknown error
        kNotFound,        //!< The hostname is unknown
        kNoAddress,        //!< The hostname is valid but has no IP address
        kUnsupported,    //!< The hostname is valid but has no supported address
        kBadPort        //!< The port is invalid
    };

    XSocketAddress(EError, const std::string& hostname, int port) noexcept;
    virtual ~XSocketAddress() noexcept { }

    //! @name accessors
    //@{

    //! Get the error code
    EError getError() const noexcept;
    //! Get the hostname
    std::string getHostname() const noexcept;
    //! Get the port
    int getPort() const noexcept;

    //@}

protected:
    // XBase overrides
    std::string getWhat() const noexcept override;

private:
    EError m_error;
    std::string m_hostname;
    int m_port;
};

//! I/O closing exception
/*!
Thrown if a stream cannot be closed.
*/
XBASE_SUBCLASS_FORMAT(XSocketIOClose, XIOClose);

//! Socket cannot bind address exception
/*!
Thrown when a socket cannot be bound to an address.
*/
XBASE_SUBCLASS_FORMAT(XSocketBind, XSocket);

//! Socket address in use exception
/*!
Thrown when a socket cannot be bound to an address because the address
is already in use.
*/
XBASE_SUBCLASS(XSocketAddressInUse, XSocketBind);

//! Cannot connect socket exception
/*!
Thrown when a socket cannot connect to a remote endpoint.
*/
XBASE_SUBCLASS_FORMAT(XSocketConnect, XSocket);

//! Cannot create socket exception
/*!
Thrown when a socket cannot be created (by the operating system).
*/
XBASE_SUBCLASS_FORMAT(XSocketCreate, XSocket);
