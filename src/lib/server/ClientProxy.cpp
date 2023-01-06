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

#include "server/ClientProxy.h"

#include "inputleap/ProtocolUtil.h"
#include "io/IStream.h"
#include "base/Log.h"
#include "base/EventQueue.h"

//
// ClientProxy
//

ClientProxy::ClientProxy(const std::string& name, inputleap::IStream* stream) :
    BaseClientProxy(name),
    m_stream(stream)
{
}

ClientProxy::~ClientProxy()
{
    delete m_stream;
}

void
ClientProxy::close(const char* msg)
{
    LOG((CLOG_DEBUG1 "send close \"%s\" to \"%s\"", msg, getName().c_str()));
    ProtocolUtil::writef(getStream(), msg);

    // force the close to be sent before we return
    getStream()->flush();
}

inputleap::IStream*
ClientProxy::getStream() const
{
    return m_stream;
}

void*
ClientProxy::getEventTarget() const
{
    return static_cast<IScreen*>(const_cast<ClientProxy*>(this));
}
