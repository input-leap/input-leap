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

#ifndef INPUTLEAP_LIB_INPUTLEAP_PLATFORM_SCREEN_LOGGING_WRAPPER_H
#define INPUTLEAP_LIB_INPUTLEAP_PLATFORM_SCREEN_LOGGING_WRAPPER_H

#include "IPlatformScreen.h"
#include <memory>

namespace inputleap {

class PlatformScreenLoggingWrapper : public IPlatformScreen
{
public:
    PlatformScreenLoggingWrapper(std::unique_ptr<IPlatformScreen> screen);

    // IPlatformScreen
    void enable() override;
    void disable() override;
    void enter() override;
    bool leave() override;
    bool setClipboard(ClipboardID id, const IClipboard* clipboard) override;
    void checkClipboards() override;
    void openScreensaver(bool notify) override;
    void closeScreensaver() override;
    void screensaver(bool activate) override;
    void resetOptions() override;
    void setOptions(const OptionsList& options) override;
    void setSequenceNumber(std::uint32_t) override;
    void setDraggingStarted(bool started) override;
    bool isPrimary() const override;

    std::string& getDraggingFilename() override;
    void clearDraggingFilename() override;
    bool isDraggingStarted() override;
    bool isFakeDraggingStarted() override;

    void fakeDraggingFiles(DragFileList file_list) override;
    const std::string& getDropTarget() const override;
    void setDropTarget(const std::string& target) override;

    // IScreen

    const EventTarget* get_event_target() const override;
    bool getClipboard(ClipboardID id, IClipboard*) const override;
    void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                  std::int32_t& height) const override;
    void getCursorPos(std::int32_t& x, std::int32_t& y) const override;

    // IPrimaryScreen

    void reconfigure(std::uint32_t active_sides) override;
    void warpCursor(std::int32_t x, std::int32_t y) override;
    std::uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override;
    void unregisterHotKey(std::uint32_t id) override;
    void fakeInputBegin() override;
    void fakeInputEnd() override;
    std::int32_t getJumpZoneSize() const override;
    bool isAnyMouseButtonDown(std::uint32_t& buttonID) const override;
    void getCursorCenter(std::int32_t& x, std::int32_t& y) const override;

    // ISecondaryScreen
    void fakeMouseButton(ButtonID id, bool press) override;
    void fakeMouseMove(std::int32_t x, std::int32_t y) override;
    void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const override;
    void fakeMouseWheel(std::int32_t x_delta, std::int32_t y_delta) const override;

    // IKeyState
    void updateKeyMap() override;
    void updateKeyState() override;
    void setHalfDuplexMask(KeyModifierMask) override;
    void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button) override;
    bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                       KeyButton button) override;
    bool fakeKeyUp(KeyButton button) override;
    void fakeAllKeysUp() override;
    bool fakeCtrlAltDel() override;
    bool fakeMediaKey(KeyID id) override;
    bool isKeyDown(KeyButton) const override;

    KeyModifierMask getActiveModifiers() const override;
    KeyModifierMask pollActiveModifiers() const override;
    std::int32_t pollActiveGroup() const override;
    void pollPressedKeys(KeyButtonSet& pressed_keys) const override;

    void handle_system_event(const Event& event) override;

private:
    std::unique_ptr<IPlatformScreen> screen_;
};

} // namespace inputleap

#endif // INPUTLEAP_LIB_INPUTLEAP_PLATFORM_SCREEN_LOGGING_WRAPPER_H
