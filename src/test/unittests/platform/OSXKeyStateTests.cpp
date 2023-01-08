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

#include "test/mock/inputleap/MockKeyMap.h"
#include "test/mock/inputleap/MockEventQueue.h"
#include "platform/OSXKeyState.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace inputleap {

TEST(OSXKeyStateTests, mapModifiersFromOSX_OSXMask_returnBarrierMask)
{
    inputleap::KeyMap keyMap;
    MockEventQueue eventQueue;
    OSXKeyState keyState(&eventQueue, keyMap);

    KeyModifierMask outMask = 0;

    std::uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
    outMask = keyState.mapModifiersFromOSX(shiftMask);
    EXPECT_EQ(KeyModifierShift, outMask);

    std::uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
    outMask = keyState.mapModifiersFromOSX(ctrlMask);
    EXPECT_EQ(KeyModifierControl, outMask);

    std::uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
    outMask = keyState.mapModifiersFromOSX(altMask);
    EXPECT_EQ(KeyModifierAlt, outMask);

    std::uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
    outMask = keyState.mapModifiersFromOSX(cmdMask);
    EXPECT_EQ(KeyModifierSuper, outMask);

    std::uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
    outMask = keyState.mapModifiersFromOSX(capsMask);
    EXPECT_EQ(KeyModifierCapsLock, outMask);

    std::uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
    outMask = keyState.mapModifiersFromOSX(numMask);
    EXPECT_EQ(KeyModifierNumLock, outMask);
}

} // namespace inputleap
