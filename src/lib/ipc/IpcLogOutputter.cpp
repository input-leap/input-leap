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

#include "ipc/IpcLogOutputter.h"

#include "ipc/IpcServer.h"
#include "ipc/IpcMessage.h"
#include "ipc/Ipc.h"
#include "ipc/IpcClientProxy.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Event.h"
#include "base/EventQueue.h"
#include "base/Time.h"

namespace inputleap {

enum EIpcLogOutputter {
    kBufferMaxSize = 1000,
    kMaxSendLines = 100,
    kBufferRateWriteLimit = 1000, // writes per kBufferRateTime
    kBufferRateTimeLimit = 1 // seconds
};

IpcLogOutputter::IpcLogOutputter(IpcServer& ipcServer, EIpcClientType clientType, bool useThread) :
    m_ipcServer(ipcServer),
    m_sending(false),
    m_bufferThread(nullptr),
    m_running(false),
    m_bufferWaiting(false),
    m_bufferThreadId(0),
    m_bufferMaxSize(kBufferMaxSize),
    m_bufferRateWriteLimit(kBufferRateWriteLimit),
    m_bufferRateTimeLimit(kBufferRateTimeLimit),
    m_bufferWriteCount(0),
    m_bufferRateStart(inputleap::current_time_seconds()),
    m_clientType(clientType)
{
    if (useThread) {
        m_bufferThread = new Thread([this](){ buffer_thread(); });
    }
}

IpcLogOutputter::~IpcLogOutputter()
{
    close();

    if (m_bufferThread != nullptr) {
        m_bufferThread->cancel();
        m_bufferThread->wait();
        delete m_bufferThread;
    }
}

void
IpcLogOutputter::open(const char* title)
{
    (void) title;
}

void
IpcLogOutputter::close()
{
    if (m_bufferThread != nullptr) {
        std::lock_guard<std::mutex> lock(m_runningMutex);
        m_running = false;
        notifyBuffer();
        m_bufferThread->wait(5);
    }
}

void
IpcLogOutputter::show(bool showIfEmpty)
{
    (void) showIfEmpty;
}

bool
IpcLogOutputter::write(ELevel, const char* text)
{
    // ignore events from the buffer thread (would cause recursion).
    if (m_bufferThread != nullptr &&
        Thread::getCurrentThread().getID() == m_bufferThreadId) {
        return true;
    }

    appendBuffer(text);
    notifyBuffer();

    return true;
}

void IpcLogOutputter::appendBuffer(const std::string& text)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    double elapsed = inputleap::current_time_seconds() - m_bufferRateStart;
    if (elapsed < m_bufferRateTimeLimit) {
        if (m_bufferWriteCount >= m_bufferRateWriteLimit) {
            // discard the log line if we've logged too much.
            return;
        }
    }
    else {
        m_bufferWriteCount = 0;
        m_bufferRateStart = inputleap::current_time_seconds();
    }

    if (m_buffer.size() >= m_bufferMaxSize) {
        // if the queue is exceeds size limit,
        // throw away the oldest item
        m_buffer.pop_front();
    }

    m_buffer.push_back(text);
    m_bufferWriteCount++;
}

bool
IpcLogOutputter::isRunning()
{
    std::lock_guard<std::mutex> lock(m_runningMutex);
    return m_running;
}

void IpcLogOutputter::buffer_thread()
{
    m_bufferThreadId = m_bufferThread->getID();
    m_running = true;

    try {
        while (isRunning()) {
            if (m_buffer.empty() || !m_ipcServer.hasClients(m_clientType)) {
                std::unique_lock<std::mutex> lock(notify_mutex_);
                ARCH->wait_cond_var(notify_cv_, lock, -1);
            }

            sendBuffer();
        }
    }
    catch (std::runtime_error& e) {
        LOG_ERR("ipc log buffer thread error, %s", e.what());
    }

    LOG_DEBUG("ipc log buffer thread finished");
}

void
IpcLogOutputter::notifyBuffer()
{
    std::lock_guard<std::mutex> lock(notify_mutex_);
    notify_cv_.notify_all();
}

std::string IpcLogOutputter::getChunk(size_t count)
{
    std::lock_guard<std::mutex> lock(m_bufferMutex);

    if (m_buffer.size() < count) {
        count = m_buffer.size();
    }

    std::string chunk;
    for (size_t i = 0; i < count; i++) {
        chunk.append(m_buffer.front());
        chunk.append("\n");
        m_buffer.pop_front();
    }
    return chunk;
}

void
IpcLogOutputter::sendBuffer()
{
    if (m_buffer.empty() || !m_ipcServer.hasClients(m_clientType)) {
        return;
    }

    IpcLogLineMessage message(getChunk(kMaxSendLines));
    m_sending = true;
    m_ipcServer.send(message, kIpcClientGui);
    m_sending = false;
}

void IpcLogOutputter::bufferMaxSize(std::uint16_t bufferMaxSize)
{
    m_bufferMaxSize = bufferMaxSize;
}

std::uint16_t IpcLogOutputter::bufferMaxSize() const
{
    return m_bufferMaxSize;
}

void IpcLogOutputter::bufferRateLimit(std::uint16_t writeLimit, double timeLimit)
{
    m_bufferRateWriteLimit = writeLimit;
    m_bufferRateTimeLimit = timeLimit;
}

} // namespace inputleap
