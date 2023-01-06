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

#include "inputleap/IKeyState.h"
#include "base/EventQueue.h"

#include <cstring>
#include <cstdlib>

//
// IKeyState
//

IKeyState::IKeyState(IEventQueue* events)
{
    (void) events;
}

//
// IKeyState::KeyInfo
//

IKeyState::KeyInfo* IKeyState::KeyInfo::alloc(KeyID id, KeyModifierMask mask, KeyButton button,
                                              std::int32_t count)
{
    KeyInfo* info = static_cast<KeyInfo*>(malloc(sizeof(KeyInfo)));
    info->m_key              = id;
    info->m_mask             = mask;
    info->m_button           = button;
    info->m_count            = count;
    info->m_screens = nullptr;
    info->m_screensBuffer[0] = '\0';
    return info;
}

IKeyState::KeyInfo* IKeyState::KeyInfo::alloc(KeyID id, KeyModifierMask mask, KeyButton button,
                                              std::int32_t count,
                                              const std::set<std::string>& destinations)
{
    std::string screens = join(destinations);

    // build structure
    KeyInfo* info = static_cast<KeyInfo*>(malloc(sizeof(KeyInfo) + screens.size()));
    info->m_key     = id;
    info->m_mask    = mask;
    info->m_button  = button;
    info->m_count   = count;
    info->m_screens = info->m_screensBuffer;
    strcpy(info->m_screensBuffer, screens.c_str());
    return info;
}

IKeyState::KeyInfo*
IKeyState::KeyInfo::alloc(const KeyInfo& x)
{
    KeyInfo* info = static_cast<KeyInfo*>(malloc(sizeof(KeyInfo) + strlen(x.m_screensBuffer)));
    info->m_key     = x.m_key;
    info->m_mask    = x.m_mask;
    info->m_button  = x.m_button;
    info->m_count   = x.m_count;
    info->m_screens = x.m_screens ? info->m_screensBuffer : nullptr;
    strcpy(info->m_screensBuffer, x.m_screensBuffer);
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
            strcmp(a->m_screensBuffer, b->m_screensBuffer) == 0);
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
