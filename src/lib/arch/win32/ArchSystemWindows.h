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

#include "arch/IArchSystem.h"

#define ARCH_SYSTEM ArchSystemWindows

//! Win32 implementation of IArchSystem
class ArchSystemWindows : public IArchSystem {
public:
    ArchSystemWindows();
    virtual ~ArchSystemWindows();

    // IArchSystem overrides
    virtual std::string getOSName() const;
    virtual std::string getPlatformName() const;
    virtual std::string setting(const std::string& valueName) const;
    virtual void setting(const std::string& valueName, const std::string& valueString) const;
    virtual std::string getLibsUsed(void) const;

    bool isWOW64() const;
};
