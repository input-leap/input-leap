/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "base/Event.h"
#include "base/EventTypes.h"
#include "base/Fwd.h"
#include <memory>

namespace inputleap {

class ClientProxy;
class IStream;
class Server;

class ClientProxyUnknown {
public:
    ClientProxyUnknown(std::unique_ptr<IStream> stream, double timeout, Server* server,
                       IEventQueue* events);
    ~ClientProxyUnknown();

    //! @name manipulators
    //@{

    //! Get the client proxy
    /*!
    Returns the client proxy created after a successful handshake
    (i.e. when this object sends a success event).  Returns nullptr
    if the handshake is unsuccessful or incomplete.
    */
    ClientProxy* orphanClientProxy();

    //! Get the stream
    inputleap::IStream* getStream() { return stream_.get(); }

    //@}

private:
    void sendSuccess();
    void sendFailure();
    void addStreamHandlers();
    void addProxyHandlers();
    void remove_handlers();
    void removeTimer();
    void handle_data();
    void handle_write_error();
    void handle_timeout();
    void handle_disconnect();
    void handle_ready();

private:
    std::unique_ptr<inputleap::IStream> stream_;
    EventQueueTimer* m_timer;
    ClientProxy* m_proxy;
    bool m_ready;
    Server* m_server;
    IEventQueue* m_events;
};

} // namespace inputleap
