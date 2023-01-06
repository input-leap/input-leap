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

#pragma once

#include "common/common.h"
#include <stdexcept>
#include <string>

//! Generic thread exception
/*!
Exceptions derived from this class are used by the multithreading
library to perform stack unwinding when a thread terminates.  These
exceptions must always be rethrown by clients when caught.
*/
class XThread { };

//! Thread exception to cancel
/*!
Thrown to cancel a thread.  Clients must not throw this type, but
must rethrow it if caught (by XThreadCancel, XThread, or ...).
*/
class XThreadCancel : public XThread { };

/*!
\def RETHROW_XTHREAD
Convenience macro to rethrow an XThread exception but ignore other
exceptions.  Put this in your catch (...) handler after necessary
cleanup but before leaving or returning from the handler.
*/
#define RETHROW_XTHREAD \
    try { throw; } catch (XThread&) { throw; } catch (...) { }

//! Generic exception architecture dependent library
class XArch : public std::runtime_error {
public:
    XArch(const std::string& msg) : std::runtime_error(msg) { }
    virtual ~XArch() noexcept { }
};

//! Generic network exception
/*!
Exceptions derived from this class are used by the networking
library to indicate various errors.
*/
class XArchNetwork : public XArch { using XArch::XArch; };

//! Operation was interrupted
class XArchNetworkInterrupted : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Network insufficient permission
class XArchNetworkAccess : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Network insufficient resources
class XArchNetworkResource : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! No support for requested network resource/service
class XArchNetworkSupport : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Network I/O error
class XArchNetworkIO : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Network address is unavailable or not local
class XArchNetworkNoAddress : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Network address in use
class XArchNetworkAddressInUse : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! No route to address
class XArchNetworkNoRoute : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Socket not connected
class XArchNetworkNotConnected : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Remote read end of socket has closed
class XArchNetworkShutdown : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Remote end of socket has disconnected
class XArchNetworkDisconnected : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Remote end of socket refused connection
class XArchNetworkConnectionRefused : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Remote end of socket is not responding
class XArchNetworkTimedOut : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! Generic network name lookup erros
class XArchNetworkName : public XArchNetwork { using XArchNetwork::XArchNetwork; };

//! The named host is unknown
class XArchNetworkNameUnknown : public XArchNetworkName {
    using XArchNetworkName::XArchNetworkName;
};

//! The named host is known but has no address
class XArchNetworkNameNoAddress : public XArchNetworkName {
    using XArchNetworkName::XArchNetworkName;
};

//! Non-recoverable name server error
class XArchNetworkNameFailure : public XArchNetworkName {
    using XArchNetworkName::XArchNetworkName;
};

//! Temporary name server error
class XArchNetworkNameUnavailable : public XArchNetworkName {
    using XArchNetworkName::XArchNetworkName;
};

//! The named host is known but no supported address
class XArchNetworkNameUnsupported : public XArchNetworkName {
    using XArchNetworkName::XArchNetworkName;
};

//! Generic daemon exception
/*!
Exceptions derived from this class are used by the daemon
library to indicate various errors.
*/
class XArchDaemon : public XArch { using XArch::XArch; };

//! Could not daemonize
class XArchDaemonFailed : public XArchDaemon { using XArchDaemon::XArchDaemon; };

//! Could not install daemon
class XArchDaemonInstallFailed : public XArchDaemon { using XArchDaemon::XArchDaemon; };

//! Could not uninstall daemon
class XArchDaemonUninstallFailed : public XArchDaemon { using XArchDaemon::XArchDaemon; };

//! Attempted to uninstall a daemon that was not installed
class XArchDaemonUninstallNotInstalled : public XArchDaemonUninstallFailed {
    using XArchDaemonUninstallFailed::XArchDaemonUninstallFailed;
};
