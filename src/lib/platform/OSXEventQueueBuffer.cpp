/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/OSXEventQueueBuffer.h"

#include "base/Event.h"
#include "base/IEventQueue.h"
#include "base/Log.h"

namespace inputleap {

OSXEventQueueBuffer::OSXEventQueueBuffer(IEventQueue* events)
    : m_eventQueue(events)
{
    // do nothing
}

OSXEventQueueBuffer::~OSXEventQueueBuffer()
{
    // Modern macOS handles dispatch queue memory management
}

void
OSXEventQueueBuffer::init()
{
    // GCD approach doesn't require initialization here
}

void
OSXEventQueueBuffer::waitForEvent(double timeout)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_dataQueue.empty()) {
        // Convert timeout (seconds) to chrono duration
        auto duration = std::chrono::duration<double>(timeout);
        LOG_DEBUG("waitForEvent waiting for events with timeout: %f seconds", timeout);
        m_cond.wait_for(lock, duration, [this]{
            return !m_dataQueue.empty();
        });
    }
    else {
        LOG_DEBUG("waitForEvent found events in the queue.");
    }
    // After waiting, getEvent() will handle the retrieved event
}

IEventQueueBuffer::Type
OSXEventQueueBuffer::getEvent(Event& event, std::uint32_t& dataID)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_dataQueue.empty()) {
        // No events available
        LOG_DEBUG("getEvent called but queue is empty. Returning kNone.");
        return kNone;
    }

    // Retrieve and remove the front event
    dataID = m_dataQueue.front();
    m_dataQueue.pop();
    lock.unlock(); // Unlock early to allow other operations

    LOG_DEBUG("Handled user event with dataID: %u", dataID);
    return kUser;
}

bool
OSXEventQueueBuffer::addEvent(std::uint32_t dataID)
{
    // Create a Syne event and enqueue it on the main queue
    dispatch_async(dispatch_get_main_queue(), ^{
        std::lock_guard<std::mutex> lock(this->m_mutex);

        LOG_DEBUG("Adding user event with dataID: %u", dataID);
        this->m_dataQueue.push(dataID);
        this->m_cond.notify_one();
        LOG_DEBUG("User event with dataID: %u added to queue.", dataID);
    });

    // dispatch_async doesn't fail under normal conditions
    return true;
}

bool
OSXEventQueueBuffer::isEmpty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    bool empty = m_dataQueue.empty();
    LOG_DEBUG("isEmpty called. Queue is %s.", empty ? "empty" : "not empty");
    return empty;
}

} // namespace inputleap
