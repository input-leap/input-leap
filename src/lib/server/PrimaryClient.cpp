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

#include "server/PrimaryClient.h"

#include "inputleap/Screen.h"
#include "inputleap/Clipboard.h"
#include "base/Log.h"

namespace inputleap {

PrimaryClient::PrimaryClient(const std::string& name, inputleap::Screen* screen) :
    BaseClientProxy(name),
    m_screen(screen),
    m_fakeInputCount(0)
{
    // all clipboards are clean
    for (std::uint32_t i = 0; i < kClipboardEnd; ++i) {
        m_clipboardDirty[i] = false;
    }
}

PrimaryClient::~PrimaryClient()
{
    // do nothing
}

void PrimaryClient::reconfigure(std::uint32_t activeSides)
{
    m_screen->reconfigure(activeSides);
}

std::uint32_t PrimaryClient::registerHotKey(KeyID key, KeyModifierMask mask)
{
    return m_screen->registerHotKey(key, mask);
}

void PrimaryClient::unregisterHotKey(std::uint32_t id)
{
    m_screen->unregisterHotKey(id);
}

void
PrimaryClient::fakeInputBegin()
{
    if (++m_fakeInputCount == 1) {
        m_screen->fakeInputBegin();
    }
}

void
PrimaryClient::fakeInputEnd()
{
    if (--m_fakeInputCount == 0) {
        m_screen->fakeInputEnd();
    }
}

std::int32_t PrimaryClient::getJumpZoneSize() const
{
    return m_screen->getJumpZoneSize();
}

void PrimaryClient::getCursorCenter(std::int32_t& x, std::int32_t& y) const
{
    m_screen->getCursorCenter(x, y);
}

KeyModifierMask
PrimaryClient::getToggleMask() const
{
    return m_screen->pollActiveModifiers();
}

bool
PrimaryClient::isLockedToScreen() const
{
    return m_screen->isLockedToScreen();
}

void*
PrimaryClient::getEventTarget() const
{
    return m_screen->getEventTarget();
}

bool
PrimaryClient::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    return m_screen->getClipboard(id, clipboard);
}

void PrimaryClient::getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                             std::int32_t& height) const
{
    m_screen->getShape(x, y, width, height);
}

void PrimaryClient::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    m_screen->getCursorPos(x, y);
}

void
PrimaryClient::enable()
{
    m_screen->enable();
}

void
PrimaryClient::disable()
{
    m_screen->disable();
}

void PrimaryClient::enter(std::int32_t xAbs, std::int32_t yAbs, std::uint32_t seqNum,
                          KeyModifierMask mask, bool screensaver)
{
    m_screen->setSequenceNumber(seqNum);
    if (!screensaver) {
        m_screen->warpCursor(xAbs, yAbs);
    }
    m_screen->enter(mask);
}

bool
PrimaryClient::leave()
{
    return m_screen->leave();
}

void
PrimaryClient::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
    // ignore if this clipboard is already clean
    if (m_clipboardDirty[id]) {
        // this clipboard is now clean
        m_clipboardDirty[id] = false;

        // set clipboard
        m_screen->setClipboard(id, clipboard);
    }
}

void
PrimaryClient::grabClipboard(ClipboardID id)
{
    // grab clipboard
    m_screen->grabClipboard(id);

    // clipboard is dirty (because someone else owns it now)
    m_clipboardDirty[id] = true;
}

void
PrimaryClient::setClipboardDirty(ClipboardID id, bool dirty)
{
    m_clipboardDirty[id] = dirty;
}

void
PrimaryClient::keyDown(KeyID key, KeyModifierMask mask, KeyButton button)
{
    if (m_fakeInputCount > 0) {
// XXX -- don't forward keystrokes to primary screen for now
        (void)key;
        (void)mask;
        (void)button;
//        m_screen->keyDown(key, mask, button);
    }
}

void PrimaryClient::keyRepeat(KeyID, KeyModifierMask, std::int32_t, KeyButton)
{
    // ignore
}

void
PrimaryClient::keyUp(KeyID key, KeyModifierMask mask, KeyButton button)
{
    if (m_fakeInputCount > 0) {
// XXX -- don't forward keystrokes to primary screen for now
        (void)key;
        (void)mask;
        (void)button;
//        m_screen->keyUp(key, mask, button);
    }
}

void
PrimaryClient::mouseDown(ButtonID)
{
    // ignore
}

void
PrimaryClient::mouseUp(ButtonID)
{
    // ignore
}

void PrimaryClient::mouseMove(std::int32_t x, std::int32_t y)
{
    m_screen->warpCursor(x, y);
}

void PrimaryClient::mouseRelativeMove(std::int32_t, std::int32_t)
{
    // ignore
}

void PrimaryClient::mouseWheel(std::int32_t, std::int32_t)
{
    // ignore
}

void
PrimaryClient::screensaver(bool)
{
    // ignore
}

void PrimaryClient::sendDragInfo(std::uint32_t fileCount, const char* info, size_t size)
{
    (void) fileCount;
    (void) info;
    (void) size;

    // ignore
}

void PrimaryClient::file_chunk_sending(const FileChunk& chunk)
{
    (void) chunk;

    // ignore
}

void
PrimaryClient::resetOptions()
{
    m_screen->resetOptions();
}

void
PrimaryClient::setOptions(const OptionsList& options)
{
    m_screen->setOptions(options);
}

} // namespace inputleap
