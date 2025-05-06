/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include <X11/X.h>

#include <cstdint>
#include <map>
#include <vector>

namespace inputleap {

//! XKB utility functions
class XKBUtil {
public:
    typedef std::vector<KeySym> KeySyms;

    //! Convert KeySym to KeyID
    /*!
    Converts a KeySym to the equivalent KeyID.  Returns kKeyNone if the
    KeySym cannot be mapped.
    */
    static std::uint32_t mapKeySymToKeyID(KeySym);

    //! Convert KeySym to corresponding KeyModifierMask
    /*!
    Converts a KeySym to the corresponding KeyModifierMask, or 0 if the
    KeySym is not a modifier.
    */
    static std::uint32_t getModifierBitForKeySym(KeySym keysym);

private:
    static void initKeyMaps();

    typedef std::map<KeySym, std::uint32_t> KeySymMap;

    static KeySymMap    s_keySymToUCS4;
};

} // namespace inputleap
