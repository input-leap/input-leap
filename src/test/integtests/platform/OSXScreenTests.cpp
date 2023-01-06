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

#include "platform/OSXScreen.h"
#include "arch/Arch.h"
#include "base/EventQueue.h"
#include "base/Time.h"

#include <gtest/gtest.h>

// disabling these tests - the return value of CGCursorIsVisible is unreliable.
/*
TEST(OSXScreenTests, hideCursor_notPrimary)
{
    EventQueue queue;
    OSXScreen screen(true, false);

    screen.hideCursor();

    EXPECT_EQ(false, CGCursorIsVisible());

    // workaround for screen class race condition.
    inputleap::this_thread_sleep(.1f);
}

TEST(OSXScreenTests, showCursor_notPrimary)
{
    EventQueue queue;
    OSXScreen screen(false, false);

    screen.showCursor();

    EXPECT_EQ(true, CGCursorIsVisible());

    // workaround for screen class race condition.
    inputleap::this_thread_sleep(.1f);
}
*/
