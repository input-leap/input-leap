/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#include "server/ClientProxy1_3.h"

namespace inputleap {

class Server;

//! Proxy for client implementing protocol version 1.4
class ClientProxy1_4 : public ClientProxy1_3 {
public:
    ClientProxy1_4(const std::string& name, inputleap::IStream* adoptedStream, Server* server,
                   IEventQueue* events);
    ~ClientProxy1_4() override;

    //! @name accessors
    //@{

    //! get server pointer
    Server* getServer() { return m_server; }

    //@}

    // IClient overrides
    void keyDown(KeyID key, KeyModifierMask mask, KeyButton button) override;
    void keyRepeat(KeyID key, KeyModifierMask mask, std::int32_t count, KeyButton button) override;
    void keyUp(KeyID key, KeyModifierMask mask, KeyButton button) override;
    void keepAlive() override;

    Server* m_server;
};

} // namespace inputleap
