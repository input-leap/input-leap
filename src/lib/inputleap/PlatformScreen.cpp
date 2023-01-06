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

#include "inputleap/PlatformScreen.h"
#include "inputleap/App.h"
#include "inputleap/ArgsBase.h"

PlatformScreen::PlatformScreen(IEventQueue* events) :
    IPlatformScreen(events),
    m_draggingStarted(false),
    m_fakeDraggingStarted(false)
{
}

PlatformScreen::~PlatformScreen()
{
    // do nothing
}

void
PlatformScreen::updateKeyMap()
{
    getKeyState()->updateKeyMap();
}

void
PlatformScreen::updateKeyState()
{
    getKeyState()->updateKeyState();
    updateButtons();
}

void
PlatformScreen::setHalfDuplexMask(KeyModifierMask mask)
{
    getKeyState()->setHalfDuplexMask(mask);
}

void
PlatformScreen::fakeKeyDown(KeyID id, KeyModifierMask mask,
                KeyButton button)
{
    getKeyState()->fakeKeyDown(id, mask, button);
}

bool PlatformScreen::fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                                   KeyButton button)
{
    return getKeyState()->fakeKeyRepeat(id, mask, count, button);
}

bool
PlatformScreen::fakeKeyUp(KeyButton button)
{
    return getKeyState()->fakeKeyUp(button);
}

void
PlatformScreen::fakeAllKeysUp()
{
    getKeyState()->fakeAllKeysUp();
}

bool
PlatformScreen::fakeCtrlAltDel()
{
    return getKeyState()->fakeCtrlAltDel();
}

bool
PlatformScreen::isKeyDown(KeyButton button) const
{
    return getKeyState()->isKeyDown(button);
}

KeyModifierMask
PlatformScreen::getActiveModifiers() const
{
    return getKeyState()->getActiveModifiers();
}

KeyModifierMask
PlatformScreen::pollActiveModifiers() const
{
    return getKeyState()->pollActiveModifiers();
}

std::int32_t PlatformScreen::pollActiveGroup() const
{
    return getKeyState()->pollActiveGroup();
}

void
PlatformScreen::pollPressedKeys(KeyButtonSet& pressedKeys) const
{
    getKeyState()->pollPressedKeys(pressedKeys);
}

bool
PlatformScreen::isDraggingStarted()
{
    if (App::instance().argsBase().m_enableDragDrop) {
        return m_draggingStarted;
    }
    return false;
}
