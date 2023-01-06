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

#include "base/Event.h"
#include "base/EventTypes.h"

class ClientProxy;
class EventQueueTimer;
namespace inputleap { class IStream; }
class Server;
class IEventQueue;

class ClientProxyUnknown {
public:
    ClientProxyUnknown(inputleap::IStream* stream, double timeout, Server* server, IEventQueue* events);
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
    inputleap::IStream* getStream() { return m_stream; }

    //@}

private:
    void sendSuccess();
    void sendFailure();
    void addStreamHandlers();
    void addProxyHandlers();
    void removeHandlers();
    void removeTimer();
    void handleData(const Event&, void*);
    void handleWriteError(const Event&, void*);
    void handleTimeout(const Event&, void*);
    void handleDisconnect(const Event&, void*);
    void handleReady(const Event&, void*);

private:
    inputleap::IStream* m_stream;
    EventQueueTimer* m_timer;
    ClientProxy* m_proxy;
    bool m_ready;
    Server* m_server;
    IEventQueue* m_events;
};
