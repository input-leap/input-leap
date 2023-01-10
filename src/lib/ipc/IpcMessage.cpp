/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"

namespace inputleap {

IpcMessage::IpcMessage(std::uint8_t type) :
    m_type(type)
{
}

IpcMessage::~IpcMessage()
{
}

IpcHelloMessage::IpcHelloMessage(EIpcClientType clientType) :
    IpcMessage(kIpcHello),
    m_clientType(clientType)
{
}

IpcHelloMessage::~IpcHelloMessage()
{
}

EventDataBase* IpcHelloMessage::clone() const
{
    return new IpcHelloMessage(*this);
}

IpcShutdownMessage::IpcShutdownMessage() :
IpcMessage(kIpcShutdown)
{
}

IpcShutdownMessage::~IpcShutdownMessage()
{
}

EventDataBase* IpcShutdownMessage::clone() const
{
    return new IpcShutdownMessage(*this);
}

IpcLogLineMessage::IpcLogLineMessage(const std::string& logLine) :
    IpcMessage(kIpcLogLine),
    m_logLine(logLine)
{
}

IpcLogLineMessage::~IpcLogLineMessage()
{
}

EventDataBase* IpcLogLineMessage::clone() const
{
    return new IpcLogLineMessage(*this);
}

IpcCommandMessage::IpcCommandMessage(const std::string& command, bool elevate) :
    IpcMessage(kIpcCommand),
    m_command(command),
    m_elevate(elevate)
{
}

IpcCommandMessage::~IpcCommandMessage()
{
}

EventDataBase* IpcCommandMessage::clone() const
{
    return new IpcCommandMessage(*this);
}

} // namespace inputleap
