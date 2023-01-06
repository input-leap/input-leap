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

#include "platform/synwinhk.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//! Loads and provides functions for the Windows hook
class MSWindowsHook
{
public:
    void setSides(std::uint32_t sides);
    void setZone(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h,
                 std::int32_t jumpZoneSize);
    void setMode(EHookMode mode);

    static bool install();
    static void uninstall();
    static bool installScreenSaver();
    static void uninstallScreenSaver();
};
