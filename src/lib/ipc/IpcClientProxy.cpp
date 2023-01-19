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

#include "ipc/IpcClientProxy.h"

#include "ipc/Ipc.h"
#include "ipc/IpcMessage.h"
#include "inputleap/ProtocolUtil.h"
#include "io/IStream.h"
#include "arch/Arch.h"
#include "base/Log.h"

namespace inputleap {

IpcClientProxy::IpcClientProxy(std::unique_ptr<IStream>&& stream, IEventQueue* events) :
    stream_(std::move(stream)),
    m_clientType(kIpcClientUnknown),
    m_disconnecting(false),
    m_events(events)
{
    m_events->add_handler(EventType::STREAM_INPUT_READY, stream_->get_event_target(),
                          [this](const auto& e){ handle_data(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_ERROR, stream_->get_event_target(),
                          [this](const auto& e){ handle_write_error(); });
    m_events->add_handler(EventType::STREAM_INPUT_SHUTDOWN, stream_->get_event_target(),
                          [this](const auto& e){ handle_disconnect(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_SHUTDOWN, stream_->get_event_target(),
                          [this](const auto& e){ handle_write_error(); });
}

IpcClientProxy::~IpcClientProxy()
{
    m_events->remove_handler(EventType::STREAM_INPUT_READY, stream_->get_event_target());
    m_events->remove_handler(EventType::STREAM_OUTPUT_ERROR, stream_->get_event_target());
    m_events->remove_handler(EventType::STREAM_INPUT_SHUTDOWN, stream_->get_event_target());
    m_events->remove_handler(EventType::STREAM_OUTPUT_SHUTDOWN, stream_->get_event_target());

    // Ensure that client proxy is not deleted from below some active client feet
    {
        std::lock_guard<std::mutex> lock_read(m_readMutex);
        std::lock_guard<std::mutex> lock_write(m_writeMutex);
    }
}

void IpcClientProxy::handle_disconnect()
{
    disconnect();
    LOG((CLOG_DEBUG "ipc client disconnected"));
}

void IpcClientProxy::handle_write_error()
{
    disconnect();
    LOG((CLOG_DEBUG "ipc client write error"));
}

void IpcClientProxy::handle_data()
{
    // don't allow the dtor to destroy the stream while we're using it.
    std::lock_guard<std::mutex> lock(m_readMutex);

    LOG((CLOG_DEBUG "start ipc handle data"));

    std::uint8_t code[4];
    std::uint32_t n = stream_->read(code, 4);
    while (n != 0) {

        LOG((CLOG_DEBUG "ipc read: %c%c%c%c",
            code[0], code[1], code[2], code[3]));

        EventDataBase* event_data = nullptr;
        if (memcmp(code, kIpcMsgHello, 4) == 0) {
            event_data = create_event_data<IpcHelloMessage>(parseHello());
        }
        else if (memcmp(code, kIpcMsgCommand, 4) == 0) {
            event_data = create_event_data<IpcCommandMessage>(parseCommand());
        }
        else {
            LOG((CLOG_ERR "invalid ipc message"));
            disconnect();
        }

        m_events->add_event(EventType::IPC_CLIENT_PROXY_MESSAGE_RECEIVED, this, event_data);

        n = stream_->read(code, 4);
    }

    LOG((CLOG_DEBUG "finished ipc handle data"));
}

void
IpcClientProxy::send(const IpcMessage& message)
{
    // don't allow other threads to write until we've finished the entire
    // message. stream write is locked, but only for that single write.
    // also, don't allow the dtor to destroy the stream while we're using it.
    std::lock_guard<std::mutex> lock(m_writeMutex);

    LOG((CLOG_DEBUG4 "ipc write: %d", message.type()));

    switch (message.type()) {
    case kIpcLogLine: {
        const IpcLogLineMessage& llm = static_cast<const IpcLogLineMessage&>(message);
        const std::string logLine = llm.logLine();
        ProtocolUtil::writef(stream_.get(), kIpcMsgLogLine, &logLine);
        break;
    }

    case kIpcShutdown:
        ProtocolUtil::writef(stream_.get(), kIpcMsgShutdown);
        break;

    default:
        LOG((CLOG_ERR "ipc message not supported: %d", message.type()));
        break;
    }
}

IpcHelloMessage IpcClientProxy::parseHello()
{
    std::uint8_t type;
    ProtocolUtil::readf(stream_.get(), kIpcMsgHello + 4, &type);

    m_clientType = static_cast<EIpcClientType>(type);

    // must be deleted by event handler.
    return IpcHelloMessage(m_clientType);
}

IpcCommandMessage IpcClientProxy::parseCommand()
{
    std::string command;
    std::uint8_t elevate;
    ProtocolUtil::readf(stream_.get(), kIpcMsgCommand + 4, &command, &elevate);

    // must be deleted by event handler.
    return IpcCommandMessage(command, elevate != 0);
}

void
IpcClientProxy::disconnect()
{
    LOG((CLOG_DEBUG "ipc disconnect, closing stream"));
    m_disconnecting = true;
    stream_->close();
    m_events->add_event(EventType::IPC_CLIENT_PROXY_DISCONNECTED, this);
}

} // namespace inputleap
