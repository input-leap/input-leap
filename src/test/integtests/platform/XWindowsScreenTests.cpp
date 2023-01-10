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

#include "test/mock/inputleap/MockEventQueue.h"
#include "platform/XWindowsScreen.h"

#include <gtest/gtest.h>
#include <cstdlib>

namespace inputleap {

using ::testing::_;

TEST(CXWindowsScreenTests, fakeMouseMove_nonPrimary_getCursorPosValuesCorrect)
{
    const char* displayName = std::getenv("DISPLAY");

    if (displayName == nullptr)
        GTEST_SKIP() << "DISPLAY environment variable not set, skipping test";

    MockEventQueue eventQueue;
    EXPECT_CALL(eventQueue, add_handler(_, _, _)).Times(2);
    EXPECT_CALL(eventQueue, adoptBuffer(_)).Times(2);
    EXPECT_CALL(eventQueue, removeHandler(_, _)).Times(2);
    XWindowsScreen screen(new XWindowsImpl(), displayName, false, 0, &eventQueue);

    screen.fakeMouseMove(10, 20);

    std::int32_t x, y;
    screen.getCursorPos(x, y);
    ASSERT_EQ(10, x);
    ASSERT_EQ(20, y);
}

} // namespace inputleap
