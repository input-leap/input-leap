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

#pragma once

#include "base/Fwd.h"
#include "base/IEventQueueBuffer.h"

#include <dispatch/dispatch.h>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace inputleap {

//! Event queue buffer for OS X
class OSXEventQueueBuffer : public IEventQueueBuffer
{
public:
    OSXEventQueueBuffer(IEventQueue* eventQueue);
    virtual ~OSXEventQueueBuffer();

    // IEventQueueBuffer overrides
    virtual void init() override;
    virtual void waitForEvent(double timeout) override;
    virtual Type getEvent(Event& event, std::uint32_t& dataID) override;
    virtual bool addEvent(std::uint32_t dataID) override;
    virtual bool isEmpty() const override;

private:
    IEventQueue* m_eventQueue;

    // Thread-safe queue of “pending” user events (the dataID)
    mutable std::mutex m_mutex;
    std::condition_variable m_cond;
    std::queue<std::uint32_t> m_dataQueue;
};

} // namespace inputleap
