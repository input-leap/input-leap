/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PlatformScreenLoggingWrapper.h"
#include "base/Log.h"

namespace inputleap {

PlatformScreenLoggingWrapper::PlatformScreenLoggingWrapper(std::unique_ptr<IPlatformScreen> screen) :
    screen_{std::move(screen)}
{}

void PlatformScreenLoggingWrapper::enable()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::enable()"));
    screen_->enable();
}

void PlatformScreenLoggingWrapper::disable()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::disable()"));
    screen_->disable();
}

void PlatformScreenLoggingWrapper::enter()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::enter()"));
    screen_->enter();
}

bool PlatformScreenLoggingWrapper::leave()
{
    auto result = screen_->leave();
    LOG((CLOG_DEBUG1 "PlatformScreen::leave() => %d", result));
    return result;
}

bool PlatformScreenLoggingWrapper::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
    bool result = screen_->setClipboard(id, clipboard);
    LOG((CLOG_DEBUG1 "PlatformScreen::setClipboard() id=%d clipboard=%p => %d",
         id, clipboard, result));
    return result;
}

void PlatformScreenLoggingWrapper::checkClipboards()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::checkClipboards()"));
    screen_->checkClipboards();
}

void PlatformScreenLoggingWrapper::openScreensaver(bool notify)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::openScreensaver() notify=%d", notify));
    screen_->openScreensaver(notify);
}

void PlatformScreenLoggingWrapper::closeScreensaver()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::closeScreensaver()"));
    screen_->closeScreensaver();
}

void PlatformScreenLoggingWrapper::screensaver(bool activate)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::screensaver() activate=%d", activate));
    screen_->screensaver(activate);
}

void PlatformScreenLoggingWrapper::resetOptions()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::resetOptions()"));
    screen_->resetOptions();
}

void PlatformScreenLoggingWrapper::setOptions(const OptionsList& options)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::setOptions() options.size()=%d",
         static_cast<int>(options.size())));
    screen_->setOptions(options);
}

void PlatformScreenLoggingWrapper::setSequenceNumber(std::uint32_t seq_num)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::setSequenceNumber() seq_num=%d", seq_num));
    screen_->setSequenceNumber(seq_num);
}

void PlatformScreenLoggingWrapper::setDraggingStarted(bool started)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::setDraggingStarted() started=%d", started));
    screen_->setDraggingStarted(started);
}

bool PlatformScreenLoggingWrapper::isPrimary() const
{
    auto result = screen_->isPrimary();
    LOG((CLOG_DEBUG1 "PlatformScreen::isPrimary() => %d", result));
    return result;
}

std::string& PlatformScreenLoggingWrapper::getDraggingFilename()
{
    auto& result = screen_->getDraggingFilename();
    LOG((CLOG_DEBUG1 "PlatformScreen::getDraggingFilename() => %s", result.c_str()));
    return result;
}

void PlatformScreenLoggingWrapper::clearDraggingFilename()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::clearDraggingFilename()"));
    screen_->clearDraggingFilename();
}

bool PlatformScreenLoggingWrapper::isDraggingStarted()
{
    auto result = screen_->isDraggingStarted();
    LOG((CLOG_DEBUG1 "PlatformScreen::isDraggingStarted() => %d", result));
    return result;
}

bool PlatformScreenLoggingWrapper::isFakeDraggingStarted()
{
    auto result = screen_->isFakeDraggingStarted();
    LOG((CLOG_DEBUG1 "PlatformScreen::isFakeDraggingStarted() => %d", result));
    return result;
}

void PlatformScreenLoggingWrapper::fakeDraggingFiles(DragFileList file_list)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeDraggingFiles() file_list.size()=%d",
         static_cast<int>(file_list.size())));
    screen_->fakeDraggingFiles(file_list);
}

const std::string& PlatformScreenLoggingWrapper::getDropTarget() const
{
    const auto& result = screen_->getDropTarget();
    LOG((CLOG_DEBUG1 "PlatformScreen::getDropTarget() => %s", result.c_str()));
    return result;
}

void PlatformScreenLoggingWrapper::setDropTarget(const std::string& target)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::setDropTarget() target=%s", target.c_str()));
    screen_->setDropTarget(target);
}

const EventTarget* PlatformScreenLoggingWrapper::get_event_target() const
{
    return screen_->get_event_target();
}

bool PlatformScreenLoggingWrapper::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    auto result = screen_->getClipboard(id, clipboard);
    LOG((CLOG_DEBUG1 "PlatformScreen::getClipboard() id=%d clipboard=%p => %d",
         id, clipboard, result));
    return result;
}

void PlatformScreenLoggingWrapper::getShape(std::int32_t& x, std::int32_t& y,
                                            std::int32_t& width, std::int32_t& height) const
{
    screen_->getShape(x, y, width, height);
    LOG((CLOG_DEBUG1 "PlatformScreen::getShape() => x=%d y=%d width=%d height=%d",
         x, y, width, height));
}

void PlatformScreenLoggingWrapper::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    screen_->getCursorPos(x, y);
    LOG((CLOG_DEBUG1 "PlatformScreen::getCursorPos() => x=%d y=%d", x, y));
}

void PlatformScreenLoggingWrapper::reconfigure(std::uint32_t active_sides)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::reconfigure() active_sides=%d", active_sides));
    screen_->reconfigure(active_sides);
}

void PlatformScreenLoggingWrapper::warpCursor(std::int32_t x, std::int32_t y)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::warpCursor() x=%d y=%d", x, y));
    screen_->warpCursor(x, y);
}

std::uint32_t PlatformScreenLoggingWrapper::registerHotKey(KeyID key, KeyModifierMask mask)
{
    auto result = screen_->registerHotKey(key, mask);
    LOG((CLOG_DEBUG1 "PlatformScreen::registerHotKey() key=%d mask=%x => %d", key, mask, result));
    return result;
}

void PlatformScreenLoggingWrapper::unregisterHotKey(std::uint32_t id)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::unregisterHotKey() id=%d", id));
    screen_->unregisterHotKey(id);
}

void PlatformScreenLoggingWrapper::fakeInputBegin()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeInputBegin()"));
    screen_->fakeInputBegin();
}

void PlatformScreenLoggingWrapper::fakeInputEnd()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeInputEnd()"));
    screen_->fakeInputEnd();
}

std::int32_t PlatformScreenLoggingWrapper::getJumpZoneSize() const
{
    auto result = screen_->getJumpZoneSize();
    LOG((CLOG_DEBUG1 "PlatformScreen::getJumpZoneSize() => %d", result));
    return result;
}

bool PlatformScreenLoggingWrapper::isAnyMouseButtonDown(std::uint32_t& button_id) const
{
    auto result = screen_->isAnyMouseButtonDown(button_id);
    LOG((CLOG_DEBUG1 "PlatformScreen::isAnyMouseButtonDown() => button_id=%d %d",
         button_id, result));
    return result;
}

void PlatformScreenLoggingWrapper::getCursorCenter(std::int32_t& x, std::int32_t& y) const
{
    screen_->getCursorCenter(x, y);
    LOG((CLOG_DEBUG1 "PlatformScreen::getCursorCenter() => x=%d y=%d", x, y));
}

void PlatformScreenLoggingWrapper::fakeMouseButton(ButtonID id, bool press)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeMouseButton() id=%d press=%d", id, press));
    screen_->fakeMouseButton(id, press);
}

void PlatformScreenLoggingWrapper::fakeMouseMove(std::int32_t x, std::int32_t y)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeMouseMove() x=%d y=%d", x, y));
    screen_->fakeMouseMove(x, y);
}

void PlatformScreenLoggingWrapper::fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeMouseRelativeMove() dx=%d dy=%d", dx, dy));
    screen_->fakeMouseRelativeMove(dx, dy);
}

void PlatformScreenLoggingWrapper::fakeMouseWheel(std::int32_t x_delta, std::int32_t y_delta) const
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeMouseWheel() x_delta=%d y_delta=%d", x_delta, y_delta));
    screen_->fakeMouseWheel(x_delta, y_delta);
}

void PlatformScreenLoggingWrapper::updateKeyMap()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::updateKeyMap()"));
    screen_->updateKeyMap();
}

void PlatformScreenLoggingWrapper::updateKeyState()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::updateKeyMap()"));
    screen_->updateKeyState();
}

void PlatformScreenLoggingWrapper::setHalfDuplexMask(KeyModifierMask mask)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::setHalfDuplexMask() mask=%x", mask));
    screen_->setHalfDuplexMask(mask);
}

void PlatformScreenLoggingWrapper::fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeKeyDown() id=%d mask=%x button=%d", id, mask, button));
    screen_->fakeKeyDown(id, mask, button);
}

bool PlatformScreenLoggingWrapper::fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                                                 KeyButton button)
{
    auto result = screen_->fakeKeyRepeat(id, mask, count, button);
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeKeyRepeat() id=%d mask=%d count=%d button=%d => %d",
         id, mask, count, button, result));
    return result;
}

bool PlatformScreenLoggingWrapper::fakeKeyUp(KeyButton button)
{
    auto result = screen_->fakeKeyUp(button);
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeKeyUp() button=%d => %d", button, result));
    return result;
}

void PlatformScreenLoggingWrapper::fakeAllKeysUp()
{
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeAllKeysUp()"));
    screen_->fakeAllKeysUp();
}

bool PlatformScreenLoggingWrapper::fakeCtrlAltDel()
{
    auto result = screen_->fakeCtrlAltDel();
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeCtrlAltDel() => %d", result));
    return result;
}

bool PlatformScreenLoggingWrapper::fakeMediaKey(KeyID id)
{
    auto result = screen_->fakeMediaKey(id);
    LOG((CLOG_DEBUG1 "PlatformScreen::fakeMediaKey() id=%d => %d", id, result));
    return result;
}

bool PlatformScreenLoggingWrapper::isKeyDown(KeyButton key) const
{
    auto result = screen_->isKeyDown(key);
    LOG((CLOG_DEBUG1 "PlatformScreen::isKeyDown() key=%d => %d", key, result));
    return result;
}

KeyModifierMask PlatformScreenLoggingWrapper::getActiveModifiers() const
{
    auto result = screen_->getActiveModifiers();
    LOG((CLOG_DEBUG1 "PlatformScreen::getActiveModifiers() => %d", result));
    return result;
}

KeyModifierMask PlatformScreenLoggingWrapper::pollActiveModifiers() const
{
    auto result = screen_->pollActiveModifiers();
    LOG((CLOG_DEBUG1 "PlatformScreen::pollActiveModifiers() => %d", result));
    return result;
}

std::int32_t PlatformScreenLoggingWrapper::pollActiveGroup() const
{
    auto result = screen_->pollActiveGroup();
    LOG((CLOG_DEBUG1 "PlatformScreen::pollActiveGroup() => %d", result));
    return result;
}

void PlatformScreenLoggingWrapper::pollPressedKeys(KeyButtonSet& pressed_keys) const
{
    screen_->pollPressedKeys(pressed_keys);
    LOG((CLOG_DEBUG1 "PlatformScreen::pollPressedKeys() => pressed_keys.size()=%d",
         pressed_keys.size()));
}

void PlatformScreenLoggingWrapper::handle_system_event(const Event& event)
{
    screen_->handle_system_event(event);
}

} // namespace inputleap
