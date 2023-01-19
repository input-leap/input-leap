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

#pragma once

#include "Fwd.h"
#include "net/IDataSocket.h"
#include "net/ISocketMultiplexerJob.h"
#include "io/StreamBuffer.h"
#include "arch/IArchNetwork.h"
#include <condition_variable>
#include <memory>
#include <mutex>

namespace inputleap {

class Thread;

//! TCP data socket
/*!
A data socket using TCP.
*/
class TCPSocket : public IDataSocket {
public:
    TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, IArchNetwork::EAddressFamily family);
    TCPSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer, ArchSocket socket);
    virtual ~TCPSocket();

    // ISocket overrides
    void bind(const NetworkAddress&) override;
    void close() override;
    const void* get_event_target() const override;

    // IStream overrides
    std::uint32_t read(void* buffer, std::uint32_t n) override;
    void write(const void* buffer, std::uint32_t n) override;
    void flush() override;
    void shutdownInput() override;
    void shutdownOutput() override;
    bool isReady() const override;
    bool isFatal() const override;
    std::uint32_t getSize() const override;

    // IDataSocket overrides
    void connect(const NetworkAddress&) override;


    virtual std::unique_ptr<ISocketMultiplexerJob> newJob();

protected:
    enum EJobResult {
        kBreak = -1,    //!< Break the Job chain
        kRetry,            //!< Retry the same job
        kNew            //!< Require a new job
    };

    ArchSocket getSocket() { return m_socket; }
    IEventQueue* getEvents() { return m_events; }
    virtual EJobResult doRead();
    virtual EJobResult doWrite();

    void removeJob();
    void setJob(std::unique_ptr<ISocketMultiplexerJob>&& job);
    MultiplexerJobStatus newJobOrStopServicing();

    bool isReadable() { return m_readable; }
    bool isWritable() { return m_writable; }

    void sendEvent(EventType type);
    void discardWrittenData(int bytesWrote);

private:
    void init();

    void sendConnectionFailedEvent(const char*);
    void onConnected();
    void onInputShutdown();
    void onOutputShutdown();
    void onDisconnected();

    MultiplexerJobStatus serviceConnecting(ISocketMultiplexerJob*, bool, bool, bool);
    MultiplexerJobStatus serviceConnected(ISocketMultiplexerJob*, bool, bool, bool);

protected:
    bool m_readable;
    bool m_writable;
    bool m_connected;
    IEventQueue* m_events;
    StreamBuffer m_inputBuffer;
    StreamBuffer m_outputBuffer;

    mutable std::mutex tcp_mutex_;
private:
    ArchSocket m_socket;
    std::condition_variable flushed_cv_;
    bool is_flushed_ = true;
    SocketMultiplexer* m_socketMultiplexer;
};

} // namespace inputleap
