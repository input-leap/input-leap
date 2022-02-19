/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2016 Symless Ltd.
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

#include "inputleap/KeyState.h"

class IOSXKeyResource {
public:
    virtual ~IOSXKeyResource() { }
    virtual bool    isValid() const = 0;
    virtual std::uint32_t getNumModifierCombinations() const = 0;
    virtual std::uint32_t getNumTables() const = 0;
    virtual std::uint32_t getNumButtons() const = 0;
    virtual std::uint32_t getTableForModifier(std::uint32_t mask) const = 0;
    virtual KeyID getKey(std::uint32_t table, std::uint32_t button) const = 0;

    // Convert a character in the current script to the equivalent KeyID
    static KeyID getKeyID(std::uint8_t);

    // Convert a unicode character to the equivalent KeyID.
    static KeyID    unicharToKeyID(UniChar);
};
