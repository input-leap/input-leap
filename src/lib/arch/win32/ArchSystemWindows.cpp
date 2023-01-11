/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "arch/win32/ArchSystemWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"

#include "tchar.h"
#include <string>

#include <windows.h>
#include <psapi.h>

#include <stdexcept>

static const char* s_settingsKeyNames[] = {
    _T("SOFTWARE"),
    _T("InputLeap"),
    nullptr
};

namespace inputleap {

ArchSystemWindows::ArchSystemWindows()
{
    // do nothing
}

ArchSystemWindows::~ArchSystemWindows()
{
    // do nothing
}

std::string
ArchSystemWindows::setting(const std::string& valueName) const
{
    HKEY key = ArchMiscWindows::openKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
    if (key == nullptr)
        return "";

    return ArchMiscWindows::readValueString(key, valueName.c_str());
}

void
ArchSystemWindows::setting(const std::string& valueName, const std::string& valueString) const
{
    HKEY key = ArchMiscWindows::addKey(HKEY_LOCAL_MACHINE, s_settingsKeyNames);
    if (key == nullptr)
        throw std::runtime_error(std::string("could not access registry key: ") + valueName);
    ArchMiscWindows::setValue(key, valueName.c_str(), valueString.c_str());
}

} // namespace inputleap
