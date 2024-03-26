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

#define INPUTLEAP_TEST_ENV

#include "config.h"

#include "test/mock/inputleap/MockKeyMap.h"
#include "test/mock/inputleap/MockEventQueue.h"
#include "platform/XWindowsKeyState.h"
#include "base/Log.h"

#define XK_LATIN1
#define XK_MISCELLANY
#include <X11/keysymdef.h>

#include <X11/XKBlib.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <errno.h>

namespace inputleap {

class XWindowsKeyStateTests : public ::testing::Test
{
protected:
    XWindowsKeyStateTests() :
        m_display(nullptr)
    {
    }

    ~XWindowsKeyStateTests() override
    {
        if (m_display != nullptr) {
            LOG_DEBUG("closing display");
            XCloseDisplay(m_display);
        }
    }

    void
    SetUp() override
    {
        // open the display only once for the entire test suite
        if (this->m_display == nullptr) {
            LOG_DEBUG("opening display");
            this->m_display = XOpenDisplay(nullptr);

            // failed to open the display and DISPLAY is null? probably
            // running in a CI, let's skip
            if (this->m_display == nullptr && std::getenv("DISPLAY") == nullptr)
                GTEST_SKIP() << "DISPLAY environment variable not set, skipping test";

            ASSERT_TRUE(this->m_display != nullptr)
                << "unable to open display: " << errno;
        }
    }

    void
    TearDown() override
    {
    }

    Display* m_display;
};

TEST_F(XWindowsKeyStateTests, setActiveGroup_pollAndSet_groupIsZero)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    keyState.setActiveGroup(XWindowsKeyState::kGroupPollAndSet);

    ASSERT_EQ(0, keyState.group());
}

TEST_F(XWindowsKeyStateTests, setActiveGroup_poll_groupIsNotSet)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    keyState.setActiveGroup(XWindowsKeyState::kGroupPoll);

    ASSERT_LE(-1, keyState.group());
}

TEST_F(XWindowsKeyStateTests, setActiveGroup_customGroup_groupWasSet)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    keyState.setActiveGroup(1);

    ASSERT_EQ(1, keyState.group());
}

TEST_F(XWindowsKeyStateTests, mapModifiersFromX_zeroState_zeroMask)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    int mask = keyState.mapModifiersFromX(0);

    ASSERT_EQ(0, mask);
}

TEST_F(XWindowsKeyStateTests, mapModifiersToX_zeroMask_resultIsTrue)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    unsigned int modifiers = 0;
    bool result = keyState.mapModifiersToX(0, modifiers);

    ASSERT_TRUE(result);
}

TEST_F(XWindowsKeyStateTests, fakeCtrlAltDel_default_returnsFalse)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    bool result = keyState.fakeCtrlAltDel();

    ASSERT_FALSE(result);
}

TEST_F(XWindowsKeyStateTests, pollActiveModifiers_defaultState_returnsZero)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    KeyModifierMask actual = keyState.pollActiveModifiers();

    ASSERT_EQ(0u, actual);
}

TEST_F(XWindowsKeyStateTests, pollActiveModifiers_shiftKeyDownThenUp_masksAreCorrect)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    // set mock modifier mapping
    std::fill(keyState.modifierFromX().begin(), keyState.modifierFromX().end(), 0);
    keyState.modifierFromX()[ShiftMapIndex] = KeyModifierShift;

    KeyCode key = XKeysymToKeycode(m_display, XK_Shift_L);

    // fake shift key down (without using InputLeap)
    XTestFakeKeyEvent(m_display, key, true, CurrentTime);

    // function under test (1st call)
    KeyModifierMask modDown = keyState.pollActiveModifiers();

    // fake shift key up (without using InputLeap)
    XTestFakeKeyEvent(m_display, key, false, CurrentTime);

    // function under test (2nd call)
    KeyModifierMask modUp = keyState.pollActiveModifiers();

    EXPECT_TRUE((modDown & KeyModifierShift) == KeyModifierShift)
        << "shift key not in mask - key was not pressed";

    EXPECT_TRUE((modUp & KeyModifierShift) == 0)
        << "shift key still in mask - make sure no keys are being held down";
}

TEST_F(XWindowsKeyStateTests, pollActiveGroup_defaultState_returnsZero)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    std::int32_t actual = keyState.pollActiveGroup();

    ASSERT_EQ(0, actual);
}

TEST_F(XWindowsKeyStateTests, pollActiveGroup_positiveGroup_returnsGroup)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    keyState.group(3);

    std::int32_t actual = keyState.pollActiveGroup();

    ASSERT_EQ(3, actual);
}

TEST_F(XWindowsKeyStateTests, pollActiveGroup_xkb_areEqual)
{
    MockKeyMap keyMap;
    MockEventQueue eventQueue;
    XWindowsKeyState keyState(new XWindowsImpl(), m_display, true, &eventQueue, keyMap);

    // reset the group
    keyState.group(-1);

    XkbStateRec state;

    // compare pollActiveGroup() with XkbGetState()
    if (XkbGetState(m_display, XkbUseCoreKbd, &state) == Success) {
        std::int32_t actual = keyState.pollActiveGroup();

        ASSERT_EQ(state.group, actual);
    }
    else {
        FAIL() << "XkbGetState() returned error " << errno;
    }
}

} // namespace inputleap
