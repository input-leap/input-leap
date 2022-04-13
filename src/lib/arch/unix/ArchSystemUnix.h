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

#pragma once

#include "config.h"

#include "arch/IArchSystem.h"

#define ARCH_SYSTEM ArchSystemUnix

//! Unix implementation of IArchSystem
class ArchSystemUnix : public IArchSystem {
public:
    ArchSystemUnix();
    ~ArchSystemUnix() override;

    // IArchSystem overrides
    std::string getOSName() const override;
    std::string getPlatformName() const override;
    std::string setting(const std::string&) const override;
    void setting(const std::string&, const std::string&) const override;
    std::string getLibsUsed(void) const override;

};
