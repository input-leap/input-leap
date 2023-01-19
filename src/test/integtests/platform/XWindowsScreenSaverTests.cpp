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

// TODO: fix tests
#if 0

#include "test/mock/inputleap/MockEventQueue.h"
#include "platform/XWindowsScreenSaver.h"

#include <gtest/gtest.h>
#include <cstdlib>
#include <X11/Xlib.h>

using ::testing::_;

// TODO: not working on build machine for some reason
TEST(CXWindowsScreenSaverTests, activate_defaultScreen_todo)
{
    const char* displayName = std::getenv("DISPLAY");
    if (displayName == nullptr) {
        displayName = ":0.0";
    }

    Display* display = XOpenDisplay(displayName);
    Window window = DefaultRootWindow(display);
    MockEventQueue eventQueue;
    EXPECT_CALL(eventQueue, remove_handler(_, _)).Times(1);
    CXWindowsScreenSaver screenSaver(
        display, window, nullptr, &eventQueue);

    screenSaver.activate();

    bool isActive = screenSaver.isActive();

    screenSaver.deactivate();

    ASSERT_EQ(true, isActive);
}
#endif
