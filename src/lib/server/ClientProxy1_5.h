/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#include "server/ClientProxy1_0.h"
#include <vector>

namespace inputleap {

class Server;
class IEventQueue;

//! Proxy for client implementing protocol version 1.5
class ClientProxy1_5 : public ClientProxy1_0 {
public:
    ClientProxy1_5(const std::string& name, inputleap::IStream* adoptedStream, Server* server,
                   IEventQueue* events);
    ~ClientProxy1_5() override;

    void sendDragInfo(std::uint32_t fileCount, const char* info, size_t size) override;
    void fileChunkSending(std::uint8_t mark, const char* data, size_t dataSize) override;
    bool parseMessage(const std::uint8_t* code) override;
    void fileChunkReceived();
    void dragInfoReceived();

private:
    IEventQueue* m_events;
};

} // namespace inputleap
