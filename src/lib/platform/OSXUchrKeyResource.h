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
#include "platform/IOSXKeyResource.h"

#include <Carbon/Carbon.h>

typedef TISInputSourceRef KeyLayout;

class OSXUchrKeyResource : public IOSXKeyResource {
public:
    OSXUchrKeyResource(const void*, std::uint32_t keyboardType);

    // KeyResource overrides
    virtual bool    isValid() const;
    virtual std::uint32_t getNumModifierCombinations() const;
    virtual std::uint32_t getNumTables() const;
    virtual std::uint32_t getNumButtons() const;
    virtual std::uint32_t getTableForModifier(std::uint32_t mask) const;
    virtual KeyID getKey(std::uint32_t table, std::uint32_t button) const;

private:
    typedef std::vector<KeyID> KeySequence;

    bool getDeadKey(KeySequence& keys, std::uint16_t index) const;
    bool getKeyRecord(KeySequence& keys, std::uint16_t index, std::uint16_t& state) const;
    bool            addSequence(KeySequence& keys, UCKeyCharSeq c) const;

private:
    const UCKeyboardLayout*            m_resource;
    const UCKeyModifiersToTableNum*    m_m;
    const UCKeyToCharTableIndex*    m_cti;
    const UCKeySequenceDataIndex*    m_sdi;
    const UCKeyStateRecordsIndex*    m_sri;
    const UCKeyStateTerminators*    m_st;
    std::uint16_t m_spaceOutput;
};
