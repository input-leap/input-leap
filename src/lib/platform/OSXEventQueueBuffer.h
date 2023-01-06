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

#pragma once

#include "base/IEventQueueBuffer.h"

#include <Carbon/Carbon.h>

class IEventQueue;

//! Event queue buffer for OS X
class OSXEventQueueBuffer : public IEventQueueBuffer {
public:
    OSXEventQueueBuffer(IEventQueue* eventQueue);
    virtual ~OSXEventQueueBuffer();

    // IEventQueueBuffer overrides
    virtual void init();
    virtual void waitForEvent(double timeout);
    virtual Type getEvent(Event& event, std::uint32_t& dataID);
    virtual bool addEvent(std::uint32_t dataID);
    virtual bool isEmpty() const;
    virtual EventQueueTimer* newTimer(double duration, bool oneShot) const;
    virtual void deleteTimer(EventQueueTimer*) const;

private:
    EventRef m_event;
    IEventQueue* m_eventQueue;
    EventQueueRef m_carbonEventQueue;
};
