/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "inputleap/Screen.h"
#include "inputleap/IPlatformScreen.h"
#include "inputleap/protocol_types.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "server/ClientProxy.h"

namespace inputleap {

Screen::Screen(std::unique_ptr<IPlatformScreen> platform_screen, IEventQueue* events) :
    m_screen(std::move(platform_screen)),
    m_isPrimary(m_screen->isPrimary()),
    m_enabled(false),
    m_entered(m_isPrimary),
    m_screenSaverSync(true),
    m_fakeInput(false),
    m_events(events),
    m_mock(false),
    m_enableDragDrop(false)
{
    // reset options
    resetOptions();

    LOG_DEBUG("opened display");
}

Screen::~Screen()
{
    if (m_mock) {
        return;
    }

    if (m_enabled) {
        disable();
    }
    assert(!m_enabled);
    assert(m_entered == m_isPrimary);
    LOG_DEBUG("closed display");
}

void
Screen::enable()
{
    assert(!m_enabled);

    m_screen->updateKeyMap();
    m_screen->updateKeyState();
    m_screen->enable();
    if (m_isPrimary) {
        enablePrimary();
    }
    else {
        enableSecondary();
    }

    // note activation
    m_enabled = true;
}

void
Screen::disable()
{
    assert(m_enabled);

    if (!m_isPrimary && m_entered) {
        leave();
    }
    else if (m_isPrimary && !m_entered) {
        enter(0);
    }
    m_screen->disable();
    if (m_isPrimary) {
        disablePrimary();
    }
    else {
        disableSecondary();
    }

    // note deactivation
    m_enabled = false;
}

void
Screen::enter(KeyModifierMask toggleMask)
{
    assert(m_entered == false);
    LOG_INFO("entering screen");

    // now on screen
    m_entered = true;

    m_screen->enter();
    if (m_isPrimary) {
        enterPrimary();
    }
    else {
        enterSecondary(toggleMask);
    }
}

bool
Screen::leave()
{
    assert(m_entered == true);
    LOG_INFO("leaving screen");

    if (!m_screen->leave()) {
        return false;
    }
    if (m_isPrimary) {
        leavePrimary();
    }
    else {
        leaveSecondary();
    }

    // make sure our idea of clipboard ownership is correct
    m_screen->checkClipboards();

    // now not on screen
    m_entered = false;

    return true;
}

void Screen::reconfigure(std::uint32_t activeSides)
{
    assert(m_isPrimary);
    m_screen->reconfigure(activeSides);
}

void Screen::warpCursor(std::int32_t x, std::int32_t y)
{
    assert(m_isPrimary);
    m_screen->warpCursor(x, y);
}

void
Screen::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
    m_screen->setClipboard(id, clipboard);
}

void
Screen::grabClipboard(ClipboardID id)
{
    m_screen->setClipboard(id, nullptr);
}

void
Screen::screensaver(bool activate)
{
    if (!m_isPrimary) {
        // activate/deactivation screen saver iff synchronization enabled
        if (m_screenSaverSync) {
            m_screen->screensaver(activate);
        }
    }
}

void
Screen::keyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
    // check for ctrl+alt+del emulation
    if (id == kKeyDelete &&
        (mask & (KeyModifierControl | KeyModifierAlt)) ==
                (KeyModifierControl | KeyModifierAlt)) {
        LOG_DEBUG("emulating ctrl+alt+del press");
        if (m_screen->fakeCtrlAltDel()) {
            return;
        }
    }
    m_screen->fakeKeyDown(id, mask, button);
}

void Screen::keyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count, KeyButton button)
{
    assert(!m_isPrimary);
    m_screen->fakeKeyRepeat(id, mask, count, button);
}

void
Screen::keyUp(KeyID, KeyModifierMask, KeyButton button)
{
    m_screen->fakeKeyUp(button);
}

void
Screen::mouseDown(ButtonID button)
{
    m_screen->fakeMouseButton(button, true);
}

void
Screen::mouseUp(ButtonID button)
{
    m_screen->fakeMouseButton(button, false);
}

void Screen::mouseMove(std::int32_t x, std::int32_t y)
{
    assert(!m_isPrimary);
    m_screen->fakeMouseMove(x, y);
}

void Screen::mouseRelativeMove(std::int32_t dx, std::int32_t dy)
{
    assert(!m_isPrimary);
    m_screen->fakeMouseRelativeMove(dx, dy);
}

void Screen::mouseWheel(std::int32_t xDelta, std::int32_t yDelta)
{
    assert(!m_isPrimary);
    m_screen->fakeMouseWheel(xDelta, yDelta);
}

void
Screen::resetOptions()
{
    // reset options
    m_halfDuplex = 0;

    // if screen saver synchronization was off then turn it on since
    // that's the default option state.
    if (!m_screenSaverSync) {
        m_screenSaverSync = true;
        if (!m_isPrimary) {
            m_screen->openScreensaver(false);
        }
    }

    // let screen handle its own options
    m_screen->resetOptions();
}

void
Screen::setOptions(const OptionsList& options)
{
    // update options
    bool oldScreenSaverSync = m_screenSaverSync;
    for (std::uint32_t i = 0, n = static_cast<std::uint32_t>(options.size()); i < n; i += 2) {
        if (options[i] == kOptionScreenSaverSync) {
            m_screenSaverSync = (options[i + 1] != 0);
            LOG_DEBUG1("screen saver synchronization %s", m_screenSaverSync ? "on" : "off");
        }
        else if (options[i] == kOptionHalfDuplexCapsLock) {
            if (options[i + 1] != 0) {
                m_halfDuplex |=  KeyModifierCapsLock;
            }
            else {
                m_halfDuplex &= ~KeyModifierCapsLock;
            }
            LOG_DEBUG1("half-duplex caps-lock %s", ((m_halfDuplex & KeyModifierCapsLock) != 0) ? "on" : "off");
        }
        else if (options[i] == kOptionHalfDuplexNumLock) {
            if (options[i + 1] != 0) {
                m_halfDuplex |=  KeyModifierNumLock;
            }
            else {
                m_halfDuplex &= ~KeyModifierNumLock;
            }
            LOG_DEBUG1("half-duplex num-lock %s", ((m_halfDuplex & KeyModifierNumLock) != 0) ? "on" : "off");
        }
        else if (options[i] == kOptionHalfDuplexScrollLock) {
            if (options[i + 1] != 0) {
                m_halfDuplex |=  KeyModifierScrollLock;
            }
            else {
                m_halfDuplex &= ~KeyModifierScrollLock;
            }
            LOG_DEBUG1("half-duplex scroll-lock %s", ((m_halfDuplex & KeyModifierScrollLock) != 0) ? "on" : "off");
        }
    }

    // update half-duplex options
    m_screen->setHalfDuplexMask(m_halfDuplex);

    // update screen saver synchronization
    if (!m_isPrimary && oldScreenSaverSync != m_screenSaverSync) {
        if (m_screenSaverSync) {
            m_screen->openScreensaver(false);
        }
        else {
            m_screen->closeScreensaver();
        }
    }

    // let screen handle its own options
    m_screen->setOptions(options);
}

void Screen::setSequenceNumber(std::uint32_t seqNum)
{
    m_screen->setSequenceNumber(seqNum);
}

std::uint32_t Screen::registerHotKey(KeyID key, KeyModifierMask mask)
{
    return m_screen->registerHotKey(key, mask);
}

void Screen::unregisterHotKey(std::uint32_t id)
{
    m_screen->unregisterHotKey(id);
}

void
Screen::fakeInputBegin()
{
    assert(!m_fakeInput);

    m_fakeInput = true;
    m_screen->fakeInputBegin();
}

void
Screen::fakeInputEnd()
{
    assert(m_fakeInput);

    m_fakeInput = false;
    m_screen->fakeInputEnd();
}

bool
Screen::isOnScreen() const
{
    return m_entered;
}

bool
Screen::isLockedToScreen() const
{
    // check for pressed mouse buttons
    // HACK: commented out as it breaks new drag drop feature
    std::uint32_t buttonID = 0;

    if (m_screen->isAnyMouseButtonDown(buttonID)) {
        if (buttonID != kButtonLeft) {
            LOG_DEBUG("locked by mouse buttonID: %d", buttonID);
        }

        if (m_enableDragDrop) {
            return (buttonID == kButtonLeft) ? false : true;
        }
        else {
            return true;
        }
    }

    // not locked
    return false;
}

std::int32_t Screen::getJumpZoneSize() const
{
    if (!m_isPrimary) {
        return 0;
    }
    else {
        return m_screen->getJumpZoneSize();
    }
}

void Screen::getCursorCenter(std::int32_t& x, std::int32_t& y) const
{
    m_screen->getCursorCenter(x, y);
}

KeyModifierMask
Screen::getActiveModifiers() const
{
    return m_screen->getActiveModifiers();
}

KeyModifierMask
Screen::pollActiveModifiers() const
{
    return m_screen->pollActiveModifiers();
}

bool
Screen::isDraggingStarted() const
{
    return m_screen->isDraggingStarted();
}

bool
Screen::isFakeDraggingStarted() const
{
    return m_screen->isFakeDraggingStarted();
}

void
Screen::setDraggingStarted(bool started)
{
    m_screen->setDraggingStarted(started);
}

void
Screen::startDraggingFiles(DragFileList& fileList)
{
    m_screen->fakeDraggingFiles(fileList);
}

void
Screen::setEnableDragDrop(bool enabled)
{
    m_enableDragDrop = enabled;
}

std::string& Screen::getDraggingFilename() const
{
    return m_screen->getDraggingFilename();
}

void
Screen::clearDraggingFilename()
{
    m_screen->clearDraggingFilename();
}

const std::string& Screen::getDropTarget() const
{
    return m_screen->getDropTarget();
}

void Screen::setDropTarget(const std::string& target)
{
    return m_screen->setDropTarget(target);
}

const EventTarget* Screen::get_event_target() const
{
    return m_screen->get_event_target();
}

bool
Screen::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    return m_screen->getClipboard(id, clipboard);
}

void Screen::getShape(std::int32_t& x, std::int32_t& y, std::int32_t& w, std::int32_t& h) const
{
    m_screen->getShape(x, y, w, h);
}

void Screen::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    m_screen->getCursorPos(x, y);
}

void
Screen::enablePrimary()
{
    // get notified of screen saver activation/deactivation
    m_screen->openScreensaver(true);

    // claim screen changed size
    m_events->add_event(EventType::SCREEN_SHAPE_CHANGED, get_event_target());
}

void
Screen::enableSecondary()
{
    // assume primary has all clipboards
    for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        grabClipboard(id);
    }

    // disable the screen saver if synchronization is enabled
    if (m_screenSaverSync) {
        m_screen->openScreensaver(false);
    }
}

void
Screen::disablePrimary()
{
    // done with screen saver
    m_screen->closeScreensaver();
}

void
Screen::disableSecondary()
{
    // done with screen saver
    m_screen->closeScreensaver();
}

void
Screen::enterPrimary()
{
    // do nothing
}

void
Screen::enterSecondary(KeyModifierMask)
{
    // do nothing
}

void
Screen::leavePrimary()
{
    // we don't track keys while on the primary screen so update our
    // idea of them now.  this is particularly to update the state of
    // the toggle modifiers.
    m_screen->updateKeyState();
}

void
Screen::leaveSecondary()
{
    // release any keys we think are still down
    m_screen->fakeAllKeysUp();
}

}
