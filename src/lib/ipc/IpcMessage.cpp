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

#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"

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

IpcShutdownMessage::IpcShutdownMessage() :
IpcMessage(kIpcShutdown)
{
}

IpcShutdownMessage::~IpcShutdownMessage()
{
}

IpcLogLineMessage::IpcLogLineMessage(const std::string& logLine) :
    IpcMessage(kIpcLogLine),
    m_logLine(logLine)
{
}

IpcLogLineMessage::~IpcLogLineMessage()
{
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
