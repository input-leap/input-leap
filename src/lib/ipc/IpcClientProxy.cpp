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

#include "ipc/IpcClientProxy.h"

#include "ipc/Ipc.h"
#include "ipc/IpcMessage.h"
#include "inputleap/ProtocolUtil.h"
#include "io/IStream.h"
#include "arch/Arch.h"
#include "base/TMethodEventJob.h"
#include "base/Log.h"

//
// IpcClientProxy
//

IpcClientProxy::IpcClientProxy(inputleap::IStream& stream, IEventQueue* events) :
    m_stream(stream),
    m_clientType(kIpcClientUnknown),
    m_disconnecting(false),
    m_events(events)
{
    m_events->adoptHandler(
        m_events->forIStream().inputReady(), stream.getEventTarget(),
        new TMethodEventJob<IpcClientProxy>(
        this, &IpcClientProxy::handleData));

    m_events->adoptHandler(
        m_events->forIStream().outputError(), stream.getEventTarget(),
        new TMethodEventJob<IpcClientProxy>(
        this, &IpcClientProxy::handleWriteError));

    m_events->adoptHandler(
        m_events->forIStream().inputShutdown(), stream.getEventTarget(),
        new TMethodEventJob<IpcClientProxy>(
        this, &IpcClientProxy::handleDisconnect));

    m_events->adoptHandler(
        m_events->forIStream().outputShutdown(), stream.getEventTarget(),
        new TMethodEventJob<IpcClientProxy>(
        this, &IpcClientProxy::handleWriteError));
}

IpcClientProxy::~IpcClientProxy()
{
    m_events->removeHandler(
        m_events->forIStream().inputReady(), m_stream.getEventTarget());
    m_events->removeHandler(
        m_events->forIStream().outputError(), m_stream.getEventTarget());
    m_events->removeHandler(
        m_events->forIStream().inputShutdown(), m_stream.getEventTarget());
    m_events->removeHandler(
        m_events->forIStream().outputShutdown(), m_stream.getEventTarget());

    // don't delete the stream while it's being used.
    {
        std::lock_guard<std::mutex> lock_read(m_readMutex);
        std::lock_guard<std::mutex> lock_write(m_writeMutex);
        delete &m_stream;
    }
}

void
IpcClientProxy::handleDisconnect(const Event&, void*)
{
    disconnect();
    LOG((CLOG_DEBUG "ipc client disconnected"));
}

void
IpcClientProxy::handleWriteError(const Event&, void*)
{
    disconnect();
    LOG((CLOG_DEBUG "ipc client write error"));
}

void
IpcClientProxy::handleData(const Event&, void*)
{
    // don't allow the dtor to destroy the stream while we're using it.
    std::lock_guard<std::mutex> lock(m_readMutex);

    LOG((CLOG_DEBUG "start ipc handle data"));

    std::uint8_t code[4];
    std::uint32_t n = m_stream.read(code, 4);
    while (n != 0) {

        LOG((CLOG_DEBUG "ipc read: %c%c%c%c",
            code[0], code[1], code[2], code[3]));

        IpcMessage* m = nullptr;
        if (memcmp(code, kIpcMsgHello, 4) == 0) {
            m = parseHello();
        }
        else if (memcmp(code, kIpcMsgCommand, 4) == 0) {
            m = parseCommand();
        }
        else {
            LOG((CLOG_ERR "invalid ipc message"));
            disconnect();
        }

        // don't delete with this event; the data is passed to a new event.
        Event e(m_events->forIpcClientProxy().messageReceived(), this, nullptr, Event::kDontFreeData);
        e.setDataObject(m);
        m_events->addEvent(e);

        n = m_stream.read(code, 4);
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
        ProtocolUtil::writef(&m_stream, kIpcMsgLogLine, &logLine);
        break;
    }

    case kIpcShutdown:
        ProtocolUtil::writef(&m_stream, kIpcMsgShutdown);
        break;

    default:
        LOG((CLOG_ERR "ipc message not supported: %d", message.type()));
        break;
    }
}

IpcHelloMessage*
IpcClientProxy::parseHello()
{
    std::uint8_t type;
    ProtocolUtil::readf(&m_stream, kIpcMsgHello + 4, &type);

    m_clientType = static_cast<EIpcClientType>(type);

    // must be deleted by event handler.
    return new IpcHelloMessage(m_clientType);
}

IpcCommandMessage*
IpcClientProxy::parseCommand()
{
    std::string command;
    std::uint8_t elevate;
    ProtocolUtil::readf(&m_stream, kIpcMsgCommand + 4, &command, &elevate);

    // must be deleted by event handler.
    return new IpcCommandMessage(command, elevate != 0);
}

void
IpcClientProxy::disconnect()
{
    LOG((CLOG_DEBUG "ipc disconnect, closing stream"));
    m_disconnecting = true;
    m_stream.close();
    m_events->addEvent(Event(m_events->forIpcClientProxy().disconnected(), this));
}
