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

#include "ipc/IpcServerProxy.h"

#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"
#include "inputleap/ProtocolUtil.h"
#include "io/IStream.h"
#include "base/Log.h"

namespace inputleap {

IpcServerProxy::IpcServerProxy(inputleap::IStream& stream, IEventQueue* events) :
    m_stream(stream),
    m_events(events)
{
    m_events->add_handler(EventType::STREAM_INPUT_READY, stream.get_event_target(),
                          [this](const auto& e){ handle_data(); });
}

IpcServerProxy::~IpcServerProxy()
{
    m_events->remove_handler(EventType::STREAM_INPUT_READY, m_stream.get_event_target());
}

void IpcServerProxy::handle_data()
{
    LOG_DEBUG("start ipc handle data");

    std::uint8_t code[4];
    std::uint32_t n = m_stream.read(code, 4);
    while (n != 0) {

        LOG_DEBUG("ipc read: %c%c%c%c",
            code[0], code[1], code[2], code[3]);

        EventDataBase* event_data = nullptr;
        if (memcmp(code, kIpcMsgLogLine, 4) == 0) {
            event_data = create_event_data<IpcLogLineMessage>(parseLogLine());
        }
        else if (memcmp(code, kIpcMsgShutdown, 4) == 0) {
            event_data = create_event_data<IpcShutdownMessage>(IpcShutdownMessage{});
        }
        else {
            LOG_ERR("invalid ipc message");
            disconnect();
        }

        m_events->add_event(EventType::IPC_SERVER_PROXY_MESSAGE_RECEIVED, this, event_data);

        n = m_stream.read(code, 4);
    }

    LOG_DEBUG("finished ipc handle data");
}

void
IpcServerProxy::send(const IpcMessage& message)
{
    LOG_DEBUG4("ipc write: %d", message.type());

    switch (message.type()) {
    case kIpcHello: {
        const IpcHelloMessage& hm = static_cast<const IpcHelloMessage&>(message);
        ProtocolUtil::writef(&m_stream, kIpcMsgHello, hm.clientType());
        break;
    }

    case kIpcCommand: {
        const IpcCommandMessage& cm = static_cast<const IpcCommandMessage&>(message);
        std::string command = cm.command();
        ProtocolUtil::writef(&m_stream, kIpcMsgCommand, &command);
        break;
    }

    default:
        LOG_ERR("ipc message not supported: %d", message.type());
        break;
    }
}

IpcLogLineMessage IpcServerProxy::parseLogLine()
{
    std::string logLine;
    ProtocolUtil::readf(&m_stream, kIpcMsgLogLine + 4, &logLine);

    // must be deleted by event handler.
    return IpcLogLineMessage(logLine);
}

void
IpcServerProxy::disconnect()
{
    LOG_DEBUG("ipc disconnect, closing stream");
    m_stream.close();
}

} // namespace inputleap
