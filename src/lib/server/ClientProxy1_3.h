/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2006 Chris Schoeneman
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

#include "server/ClientProxy1_2.h"

//! Proxy for client implementing protocol version 1.3
class ClientProxy1_3 : public ClientProxy1_2 {
public:
    ClientProxy1_3(const std::string& name, inputleap::IStream* adoptedStream, IEventQueue* events);
    ~ClientProxy1_3() override;

    // IClient overrides
    void mouseWheel(std::int32_t xDelta, std::int32_t yDelta) override;

    void                handleKeepAlive(const Event&, void*);

protected:
    // ClientProxy overrides
    bool parseMessage(const std::uint8_t* code) override;
    void resetHeartbeatRate() override;
    void setHeartbeatRate(double rate, double alarm) override;
    void resetHeartbeatTimer() override;
    void addHeartbeatTimer() override;
    void removeHeartbeatTimer() override;
    virtual void        keepAlive();

private:
    double m_keepAliveRate;
    EventQueueTimer* m_keepAliveTimer;
    IEventQueue* m_events;
};
