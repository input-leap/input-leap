/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "net/TCPSocket.h"

#include "net/NetworkAddress.h"
#include "net/SocketMultiplexer.h"
#include "net/TSocketMultiplexerMethodJob.h"
#include "net/XSocket.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/IEventQueue.h"

#include <cstring>
#include <cstdlib>
#include <memory>

namespace inputleap {

static const std::size_t MAX_INPUT_BUFFER_SIZE = 1024 * 1024;

TCPSocket::TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family) :
    IDataSocket(events),
    m_events(events),
    m_socketMultiplexer(socketMultiplexer)
{
    try {
        m_socket = ARCH->newSocket(family, IArchNetwork::kSTREAM);
    }
    catch (XArchNetwork& e) {
        throw XSocketCreate(e.what());
    }

    LOG_DEBUG("Opening new socket: %p", m_socket);

    init();
}

TCPSocket::TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, ArchSocket socket) :
    IDataSocket(events),
    m_events(events),
    m_socket(socket),
    m_socketMultiplexer(socketMultiplexer)
{
    assert(m_socket != nullptr);

    LOG_DEBUG("Opening new socket: %p", m_socket);

    // socket starts in connected state
    init();
    onConnected();
    setJob(newJob());
}

TCPSocket::~TCPSocket()
{
    try {
        close();
    }
    catch (...) {
        // ignore
    }
}

void
TCPSocket::bind(const NetworkAddress& addr)
{
    try {
        ARCH->bindSocket(m_socket, addr.getAddress());
    }
    catch (XArchNetworkAddressInUse& e) {
        throw XSocketAddressInUse(e.what());
    }
    catch (XArchNetwork& e) {
        throw XSocketBind(e.what());
    }
}

void
TCPSocket::close()
{
    LOG_DEBUG("Closing socket: %p", m_socket);

    // remove ourself from the multiplexer
    setJob(nullptr);

    std::lock_guard<std::mutex> lock(tcp_mutex_);

    // clear buffers and enter disconnected state
    if (m_connected) {
        sendEvent(EventType::SOCKET_DISCONNECTED);
    }
    onDisconnected();

    // close the socket
    if (m_socket != nullptr) {
        ArchSocket socket = m_socket;
        m_socket = nullptr;
        try {
            ARCH->closeSocket(socket);
        }
        catch (XArchNetwork& e) {
            // ignore, there's not much we can do
            LOG_WARN("error closing socket: %s", e.what());
        }
    }
}

const EventTarget* TCPSocket::get_event_target() const
{
    return this;
}

std::uint32_t TCPSocket::read(void* buffer, std::uint32_t n)
{
    // copy data directly from our input buffer
    std::lock_guard<std::mutex> lock(tcp_mutex_);
    std::uint32_t size = m_inputBuffer.getSize();
    if (n > size) {
        n = size;
    }
    if (buffer != nullptr && n != 0) {
        memcpy(buffer, m_inputBuffer.peek(n), n);
    }
    m_inputBuffer.pop(n);

    // if no more data and we cannot read or write then send disconnected
    if (n > 0 && m_inputBuffer.getSize() == 0 && !m_readable && !m_writable) {
        sendEvent(EventType::SOCKET_DISCONNECTED);
        m_connected = false;
    }

    return n;
}

void TCPSocket::write(const void* buffer, std::uint32_t n)
{
    bool wasEmpty;
    {
        std::lock_guard<std::mutex> lock(tcp_mutex_);

        // must not have shutdown output
        if (!m_writable) {
            sendEvent(EventType::STREAM_OUTPUT_ERROR);
            return;
        }

        // ignore empty writes
        if (n == 0) {
            return;
        }

        // copy data to the output buffer
        wasEmpty = (m_outputBuffer.getSize() == 0);
        m_outputBuffer.write(buffer, n);

        // there's data to write
        is_flushed_ = false;
    }

    // make sure we're waiting to write
    if (wasEmpty) {
        setJob(newJob());
    }
}

void
TCPSocket::flush()
{
    std::unique_lock<std::mutex> lock(tcp_mutex_);
    flushed_cv_.wait(lock, [this](){ return is_flushed_; });
}

void
TCPSocket::shutdownInput()
{
    bool useNewJob = false;
    {
        std::lock_guard<std::mutex> lock(tcp_mutex_);

        // shutdown socket for reading
        try {
            ARCH->closeSocketForRead(m_socket);
        }
        catch (XArchNetwork&) {
            // ignore
        }

        // shutdown buffer for reading
        if (m_readable) {
            sendEvent(EventType::STREAM_INPUT_SHUTDOWN);
            onInputShutdown();
            useNewJob = true;
        }
    }
    if (useNewJob) {
        setJob(newJob());
    }
}

void
TCPSocket::shutdownOutput()
{
    bool useNewJob = false;
    {
        std::lock_guard<std::mutex> lock(tcp_mutex_);

        // shutdown socket for writing
        try {
            ARCH->closeSocketForWrite(m_socket);
        }
        catch (XArchNetwork&) {
            // ignore
        }

        // shutdown buffer for writing
        if (m_writable) {
            sendEvent(EventType::STREAM_OUTPUT_SHUTDOWN);
            onOutputShutdown();
            useNewJob = true;
        }
    }
    if (useNewJob) {
        setJob(newJob());
    }
}

bool
TCPSocket::isReady() const
{
    std::lock_guard<std::mutex> lock(tcp_mutex_);
    return (m_inputBuffer.getSize() > 0);
}

bool
TCPSocket::isFatal() const
{
    // TCP sockets aren't ever left in a fatal state.
    LOG_ERR("isFatal() not valid for non-secure connections");
    return false;
}

std::uint32_t TCPSocket::getSize() const
{
    std::lock_guard<std::mutex> lock(tcp_mutex_);
    return m_inputBuffer.getSize();
}

void
TCPSocket::connect(const NetworkAddress& addr)
{
    {
        std::lock_guard<std::mutex> lock(tcp_mutex_);

        // fail on attempts to reconnect
        if (m_socket == nullptr || m_connected) {
            sendConnectionFailedEvent("busy");
            return;
        }

        try {
            if (ARCH->connectSocket(m_socket, addr.getAddress())) {
                sendEvent(EventType::DATA_SOCKET_CONNECTED);
                onConnected();
            }
            else {
                // connection is in progress
                m_writable = true;
            }
        }
        catch (XArchNetwork& e) {
            throw XSocketConnect(e.what());
        }
    }
    setJob(newJob());
}

void
TCPSocket::init()
{
    // default state
    m_connected = false;
    m_readable  = false;
    m_writable  = false;

    try {
        // turn off Nagle algorithm.  we send lots of very short messages
        // that should be sent without (much) delay.  for example, the
        // mouse motion messages are much less useful if they're delayed.
        ARCH->setNoDelayOnSocket(m_socket, true);
    }
    catch (XArchNetwork& e) {
        try {
            ARCH->closeSocket(m_socket);
            m_socket = nullptr;
        }
        catch (XArchNetwork&) {
            // ignore
        }
        throw XSocketCreate(e.what());
    }
}

TCPSocket::EJobResult
TCPSocket::doRead()
{
    std::uint8_t buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    size_t bytesRead = 0;

    bytesRead = ARCH->readSocket(m_socket, buffer, sizeof(buffer));

    if (bytesRead > 0) {
        bool wasEmpty = (m_inputBuffer.getSize() == 0);

        // slurp up as much as possible
        do {
            m_inputBuffer.write(buffer, static_cast<std::uint32_t>(bytesRead));

            if (m_inputBuffer.getSize() > MAX_INPUT_BUFFER_SIZE) {
                break;
            }

            bytesRead = ARCH->readSocket(m_socket, buffer, sizeof(buffer));
        } while (bytesRead > 0);

        // send input ready if input buffer was empty
        if (wasEmpty) {
            sendEvent(EventType::STREAM_INPUT_READY);
        }
    }
    else {
        // remote write end of stream hungup.  our input side
        // has therefore shutdown but don't flush our buffer
        // since there's still data to be read.
        sendEvent(EventType::STREAM_INPUT_SHUTDOWN);
        if (!m_writable && m_inputBuffer.getSize() == 0) {
            sendEvent(EventType::SOCKET_DISCONNECTED);
            m_connected = false;
        }
        m_readable = false;
        return kNew;
    }

    return kRetry;
}

TCPSocket::EJobResult
TCPSocket::doWrite()
{
    // write data
    std::uint32_t bufferSize = 0;
    int bytesWrote = 0;

    bufferSize = m_outputBuffer.getSize();
    const void* buffer = m_outputBuffer.peek(bufferSize);
    bytesWrote = static_cast<std::uint32_t>(ARCH->writeSocket(m_socket, buffer, bufferSize));

    if (bytesWrote > 0) {
        discardWrittenData(bytesWrote);
        return kNew;
    }

    return kRetry;
}

void TCPSocket::removeJob()
{
    // multiplexer will delete the old job
    m_socketMultiplexer->removeSocket(this);
}

void TCPSocket::setJob(std::unique_ptr<ISocketMultiplexerJob>&& job)
{
    if (job.get() == nullptr) {
        removeJob();
    } else {
        m_socketMultiplexer->addSocket(this, std::move(job));
    }
}

MultiplexerJobStatus TCPSocket::newJobOrStopServicing()
{
    auto new_job = newJob();
    if (new_job)
        return {true, std::move(new_job)};
    else
        return {false, {}};
}

std::unique_ptr<ISocketMultiplexerJob> TCPSocket::newJob()
{
    // note -- must have m_mutex locked on entry

    if (m_socket == nullptr) {
        return {};
    }
    else if (!m_connected) {
        assert(!m_readable);
        if (!(m_readable || m_writable)) {
            return {};
        }
        return std::make_unique<TSocketMultiplexerMethodJob>(
                    [this](auto j, auto r, auto w, auto e)
                    { return serviceConnecting(j, r, w, e); },
                    m_socket, m_readable, m_writable);
    }
    else {
        auto writable = m_writable && (m_outputBuffer.getSize() > 0);
        if (!(m_readable || writable)) {
            return {};
        }
        return std::make_unique<TSocketMultiplexerMethodJob>(
                    [this](auto j, auto r, auto w, auto e)
                    { return serviceConnected(j, r, w, e); },
                    m_socket, m_readable, writable);
    }
}

void
TCPSocket::sendConnectionFailedEvent(const char* msg)
{
    ConnectionFailedInfo info{msg};
    m_events->add_event(EventType::DATA_SOCKET_CONNECTION_FAILED, get_event_target(),
                        create_event_data<ConnectionFailedInfo>(info));
}

void TCPSocket::sendEvent(EventType type)
{
    m_events->add_event(type, get_event_target());
}

void
TCPSocket::discardWrittenData(int bytesWrote)
{
    m_outputBuffer.pop(bytesWrote);
    if (m_outputBuffer.getSize() == 0) {
        sendEvent(EventType::STREAM_OUTPUT_FLUSHED);
        is_flushed_ = true;
        flushed_cv_.notify_all();
    }
}

void
TCPSocket::onConnected()
{
    m_connected = true;
    m_readable  = true;
    m_writable  = true;
}

void
TCPSocket::onInputShutdown()
{
    m_inputBuffer.pop(m_inputBuffer.getSize());
    m_readable = false;
}

void
TCPSocket::onOutputShutdown()
{
    m_outputBuffer.pop(m_outputBuffer.getSize());
    m_writable = false;

    // we're now flushed
    is_flushed_ = true;
    flushed_cv_.notify_all();
}

void
TCPSocket::onDisconnected()
{
    // disconnected
    onInputShutdown();
    onOutputShutdown();
    m_connected = false;
}

MultiplexerJobStatus TCPSocket::serviceConnecting(ISocketMultiplexerJob* job, bool, bool write, bool error)
{
    (void) job;

    std::lock_guard<std::mutex> lock(tcp_mutex_);

    // should only check for errors if error is true but checking a new
    // socket (and a socket that's connecting should be new) for errors
    // should be safe and Mac OS X appears to have a bug where a
    // non-blocking stream socket that fails to connect immediately is
    // reported by select as being writable (i.e. connected) even when
    // the connection has failed.  this is easily demonstrated on OS X
    // 10.3.4 by starting a InputLeap client and telling to connect to
    // another system that's not running a InputLeap server.  it will
    // claim to have connected then quickly disconnect (i guess because
    // read returns 0 bytes).  unfortunately, InputLeap attempts to
    // reconnect immediately, the process repeats and we end up
    // spinning the CPU.  luckily, OS X does set SO_ERROR on the
    // socket correctly when the connection has failed so checking for
    // errors works.  (curiously, sometimes OS X doesn't report
    // connection refused.  when that happens it at least doesn't
    // report the socket as being writable so InputLeap is able to time
    // out the attempt.)
    if (error || true) {
        try {
            // connection may have failed or succeeded
            ARCH->throwErrorOnSocket(m_socket);
        }
        catch (XArchNetwork& e) {
            sendConnectionFailedEvent(e.what());
            onDisconnected();
            return newJobOrStopServicing();
        }
    }

    if (write) {
        sendEvent(EventType::DATA_SOCKET_CONNECTED);
        onConnected();
        return newJobOrStopServicing();
    }

    return {true, {}};
}

MultiplexerJobStatus TCPSocket::serviceConnected(ISocketMultiplexerJob* job,
                                                 bool read, bool write, bool error)
{
    (void) job;

    std::lock_guard<std::mutex> lock(tcp_mutex_);

    if (error) {
        sendEvent(EventType::SOCKET_DISCONNECTED);
        onDisconnected();
        return newJobOrStopServicing();
    }

    EJobResult writeResult = kRetry;
    EJobResult readResult = kRetry;
    if (write) {
        try {
            writeResult = doWrite();
        }
        catch (XArchNetworkShutdown&) {
            // remote read end of stream hungup.  our output side
            // has therefore shutdown.
            onOutputShutdown();
            sendEvent(EventType::STREAM_OUTPUT_SHUTDOWN);
            if (!m_readable && m_inputBuffer.getSize() == 0) {
                sendEvent(EventType::SOCKET_DISCONNECTED);
                m_connected = false;
            }
            writeResult = kNew;
        }
        catch (XArchNetworkDisconnected&) {
            // stream hungup
            onDisconnected();
            sendEvent(EventType::SOCKET_DISCONNECTED);
            writeResult = kNew;
        }
        catch (XArchNetwork& e) {
            // other write error
            LOG_WARN("error writing socket: %s", e.what());
            onDisconnected();
            sendEvent(EventType::STREAM_OUTPUT_ERROR);
            sendEvent(EventType::SOCKET_DISCONNECTED);
            writeResult = kNew;
        }
    }

    if (read && m_readable) {
        try {
            readResult = doRead();
        }
        catch (XArchNetworkDisconnected&) {
            // stream hungup
            sendEvent(EventType::SOCKET_DISCONNECTED);
            onDisconnected();
            readResult = kNew;
        }
        catch (XArchNetwork& e) {
            // ignore other read error
            LOG_WARN("error reading socket: %s", e.what());
        }
    }

    if (writeResult == kBreak || readResult == kBreak) {
        return {false, {}};
    } else if (writeResult == kNew || readResult == kNew) {
        return newJobOrStopServicing();
    } else {
        return {true, {}};
    }
}

} // namespace inputleap
