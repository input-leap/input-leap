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

#include "inputleap/KeyMap.h"

#include <gmock/gmock.h>

class MockKeyMap : public inputleap::KeyMap
{
public:
    MOCK_METHOD1(swap, void(KeyMap&));
    MOCK_METHOD0(finish, void());
    MOCK_METHOD2(foreachKey, void(ForeachKeyCallback, void*));
    MOCK_METHOD1(addHalfDuplexModifier, void(KeyID));
    MOCK_CONST_METHOD2(isHalfDuplex, bool(KeyID, KeyButton));
    MOCK_CONST_METHOD7(mapKey, const KeyMap::KeyItem*(Keystrokes&, KeyID, std::int32_t,
                                                      ModifierToKeys&, KeyModifierMask&,
                                                      KeyModifierMask, bool));
};
