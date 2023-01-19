/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include "base/IEventQueue.h"

#include <gmock/gmock.h>

namespace inputleap {

class MockEventQueue : public IEventQueue
{
public:
    MOCK_METHOD0(loop, void());
    MOCK_METHOD2(newOneShotTimer, EventQueueTimer*(double, void*));
    MOCK_METHOD2(newTimer, EventQueueTimer*(double, void*));
    MOCK_METHOD2(getEvent, bool(Event&, double));
    MOCK_METHOD1(adoptBuffer, void(IEventQueueBuffer*));
    MOCK_METHOD1(remove_handlers, void(const void*));
    MOCK_METHOD1(registerType, EventType(const char*));
    MOCK_CONST_METHOD0(isEmpty, bool());
    MOCK_METHOD3(add_handler, void(EventType, const void*, const EventHandler&));
    MOCK_METHOD1(add_event, void(Event&&));
    MOCK_METHOD2(remove_handler, void(EventType, const void*));
    MOCK_METHOD1(dispatchEvent, bool(const Event&));
    MOCK_METHOD1(deleteTimer, void(EventQueueTimer*));
    MOCK_METHOD0(getSystemTarget, void*());
    MOCK_CONST_METHOD0(waitForReady, void());
};

} // namespace inputleap
