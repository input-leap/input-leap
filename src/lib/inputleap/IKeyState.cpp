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

#include "inputleap/IKeyState.h"
#include "base/EventQueue.h"

#include <cstring>
#include <cstdlib>

namespace inputleap {

IKeyState::KeyInfo IKeyState::KeyInfo::create(KeyID id, KeyModifierMask mask, KeyButton button,
                                              std::int32_t count,
                                              const std::set<std::string>& destinations)
{
    KeyInfo info;
    info.m_key = id;
    info.m_mask = mask;
    info.m_button = button;
    info.m_count = count;
    info.screens_ = join(destinations);
    return info;
}

bool
IKeyState::KeyInfo::isDefault(const char* screens)
{
    return (screens == nullptr || screens[0] == '\0');
}

bool
IKeyState::KeyInfo::contains(const char* screens, const std::string& name)
{
    // special cases
    if (isDefault(screens)) {
        return false;
    }
    if (screens[0] == '*') {
        return true;
    }

    // search
    std::string match;
    match.reserve(name.size() + 2);
    match += ":";
    match += name;
    match += ":";
    return (strstr(screens, match.c_str()) != nullptr);
}

bool
IKeyState::KeyInfo::equal(const KeyInfo* a, const KeyInfo* b)
{
    return (a->m_key    == b->m_key &&
            a->m_mask   == b->m_mask &&
            a->m_button == b->m_button &&
            a->m_count  == b->m_count &&
            a->screens_ == b->screens_);
}

std::string IKeyState::KeyInfo::join(const std::set<std::string>& destinations)
{
    // collect destinations into a string.  names are surrounded by ':'
    // which makes searching easy.  the string is empty if there are no
    // destinations and "*" means all destinations.
    std::string screens;
    for (auto i = destinations.begin(); i != destinations.end(); ++i) {
        if (*i == "*") {
            screens = "*";
            break;
        }
        else {
            if (screens.empty()) {
                screens = ":";
            }
            screens += *i;
            screens += ":";
        }
    }
    return screens;
}

void IKeyState::KeyInfo::split(const char* screens, std::set<std::string>& dst)
{
    dst.clear();
    if (isDefault(screens)) {
        return;
    }
    if (screens[0] == '*') {
        dst.insert("*");
        return;
    }

    const char* i = screens + 1;
    while (*i != '\0') {
        const char* j = strchr(i, ':');
        dst.insert(std::string(i, j - i));
        i = j + 1;
    }
}

} // namespace inputleap
