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

#include "server/BaseClientProxy.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include <memory>

namespace inputleap {

//! Generic proxy for client
class ClientProxy : public BaseClientProxy {
public:
    /*!
    \c name is the name of the client.
    */
    ClientProxy(const std::string& name, std::unique_ptr<IClientConnection> backend);
    ~ClientProxy();

    //! Disconnect
    /*!
    Ask the client to disconnect, using \p msg as the reason.
    */
    void close(const char* msg);

    /// Returns original IClientProtocolWriter passed to ctor
    IClientConnection& get_conn() const override { return *conn_; }

    // IScreen
    const void* get_event_target() const override;

private:
    std::unique_ptr<IClientConnection> conn_;
};

} // namespace inputleap
