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

#include "server/ClientProxy1_4.h"

#include "server/Server.h"
#include "inputleap/ProtocolUtil.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

#include <cstring>
#include <memory>

//
// ClientProxy1_4
//

ClientProxy1_4::ClientProxy1_4(const std::string& name, inputleap::IStream* stream, Server* server,
                               IEventQueue* events) :
    ClientProxy1_3(name, stream, events), m_server(server)
{
    assert(m_server != nullptr);
}

ClientProxy1_4::~ClientProxy1_4()
{
}

void
ClientProxy1_4::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
    ClientProxy1_3::keyDown(key, mask, button);
}

void ClientProxy1_4::keyRepeat(KeyID key, KeyModifierMask mask, std::int32_t count,
                               KeyButton button)
{
    ClientProxy1_3::keyRepeat(key, mask, count, button);
}

void
ClientProxy1_4::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
    ClientProxy1_3::keyUp(key, mask, button);
}

void
ClientProxy1_4::keepAlive()
{
    ClientProxy1_3::keepAlive();
}
