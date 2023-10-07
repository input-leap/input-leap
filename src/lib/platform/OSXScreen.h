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

#include "platform/OSXClipboard.h"
#include "inputleap/PlatformScreen.h"
#include "inputleap/DragInformation.h"
#include "base/EventTypes.h"
#include "base/Fwd.h"

#include <Carbon/Carbon.h>
#include <mach/mach_port.h>
#include <mach/mach_interface.h>
#include <mach/mach_init.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>

#include <condition_variable>
#include <bitset>
#include <map>
#include <mutex>
#include <vector>

namespace inputleap {

extern "C" {
    typedef int CGSConnectionID;
    CGError CGSSetConnectionProperty(CGSConnectionID cid, CGSConnectionID targetCID, CFStringRef key, CFTypeRef value);
    int _CGSDefaultConnection();
}

class Thread;
class OSXKeyState;
class OSXScreenSaver;

//! Implementation of IPlatformScreen for OS X
class OSXScreen : public PlatformScreen {
public:
    OSXScreen(IEventQueue* events, bool isPrimary, bool autoShowHideCursor=true);
    virtual ~OSXScreen();

    IEventQueue* getEvents() const { return m_events; }

    // IScreen overrides
    virtual const EventTarget* get_event_target() const;
    virtual bool getClipboard(ClipboardID id, IClipboard*) const;
    virtual void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                          std::int32_t& height) const;
    virtual void getCursorPos(std::int32_t& x, std::int32_t& y) const;

    // IPrimaryScreen overrides
    virtual void reconfigure(std::uint32_t activeSides);
    virtual void warpCursor(std::int32_t x, std::int32_t y);
    virtual std::uint32_t registerHotKey(KeyID key, KeyModifierMask mask);
    virtual void unregisterHotKey(std::uint32_t id);
    virtual void fakeInputBegin();
    virtual void fakeInputEnd();
    virtual std::int32_t getJumpZoneSize() const;
    virtual bool isAnyMouseButtonDown(std::uint32_t& buttonID) const;
    virtual void getCursorCenter(std::int32_t& x, std::int32_t& y) const;

    // ISecondaryScreen overrides
    virtual void fakeMouseButton(ButtonID id, bool press);
    virtual void fakeMouseMove(std::int32_t x, std::int32_t y);
    virtual void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const;
    virtual void fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const;

    // IPlatformScreen overrides
    virtual void enable();
    virtual void disable();
    virtual void enter();
    virtual bool leave();
    virtual bool setClipboard(ClipboardID, const IClipboard*);
    virtual void checkClipboards();
    virtual void openScreensaver(bool notify);
    virtual void closeScreensaver();
    virtual void screensaver(bool activate);
    virtual void resetOptions();
    virtual void setOptions(const OptionsList& options);
    virtual void setSequenceNumber(std::uint32_t);
    virtual bool isPrimary() const;
    virtual void fakeDraggingFiles(DragFileList fileList);
    virtual std::string& getDraggingFilename();

    const std::string& getDropTarget() const { return m_dropTarget; }
    void waitForCarbonLoop() const;

protected:
    // IPlatformScreen overrides
    virtual void handle_system_event(const Event& event);
    virtual void updateButtons();
    virtual IKeyState* getKeyState() const;

private:
    void updateScreenShape();
    void updateScreenShape(const CGDirectDisplayID, const CGDisplayChangeSummaryFlags);
    void postMouseEvent(CGPoint&) const;

    // convenience function to send events
    void sendEvent(EventType type, EventDataBase* = nullptr) const;
    void sendClipboardEvent(EventType type, ClipboardID id) const;

    // message handlers
    bool onMouseMove(CGFloat mx, CGFloat my);
    // mouse button handler.  pressed is true if this is a mousedown
    // event, false if it is a mouseup event.  macButton is the index
    // of the button pressed using the mac button mapping.
    bool onMouseButton(bool pressed, std::uint16_t macButton);
    bool onMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const;

    void constructMouseButtonEventMap();

    bool onKey(CGEventRef event);

    void onMediaKey(CGEventRef event);

    bool onHotKey(EventRef event) const;

    // Added here to allow the carbon cursor hack to be called.
    void showCursor();
    void hideCursor();

    // map InputLeap mouse button to mac buttons
    ButtonID map_button_to_osx(std::uint16_t) const;

    // map mac mouse button to InputLeap buttons
    ButtonID map_button_from_osx(std::uint16_t) const;

    // map mac scroll wheel value to a InputLeap scroll wheel value
    std::int32_t map_scroll_wheel_from_osx(float) const;

    // map InputLeap scroll wheel value to a mac scroll wheel value
    std::int32_t map_scroll_wheel_to_osx(float) const;

    // get the current scroll wheel speed
    double getScrollSpeed() const;

    // get the current scroll wheel speed
    double getScrollSpeedFactor() const;

    // enable/disable drag handling for buttons 3 and up
    void enableDragTimer(bool enable);

    // drag timer handler
    void handle_drag();

    // clipboard check timer handler
    void handle_clipboard_check();

    // Resolution switch callback
    static void displayReconfigurationCallback(CGDirectDisplayID,
                            CGDisplayChangeSummaryFlags, void*);

    // fast user switch callback
    static pascal OSStatus
                        userSwitchCallback(EventHandlerCallRef nextHandler,
                           EventRef theEvent, void* inUserData);

    // sleep / wakeup support
    void watchSystemPowerThread();
    static void testCanceled(CFRunLoopTimerRef timer, void*info);
    static void powerChangeCallback(void* refcon, io_service_t service,
                            natural_t messageType, void* messageArgument);
    void handlePowerChangeRequest(natural_t messageType,
                             void* messageArgument);

    void handle_confirm_sleep(const Event& event);

    // global hotkey operating mode
    static bool isGlobalHotKeyOperatingModeAvailable();
    static void setGlobalHotKeysEnabled(bool enabled);
    static bool getGlobalHotKeysEnabled();

    // Quartz event tap support
    static CGEventRef handleCGInputEvent(CGEventTapProxy proxy,
                                           CGEventType type,
                                           CGEventRef event,
                                           void* refcon);
    static CGEventRef handleCGInputEventSecondary(CGEventTapProxy proxy,
                                                    CGEventType type,
                                                    CGEventRef event,
                                                    void* refcon);

    // convert CFString to char*
    static char*        CFStringRefToUTF8String(CFStringRef aString);

    void get_drop_target_thread();

private:
    struct HotKeyItem {
    public:
        HotKeyItem(std::uint32_t, std::uint32_t);
        HotKeyItem(EventHotKeyRef, std::uint32_t, std::uint32_t);

        EventHotKeyRef getRef() const;

        bool            operator<(const HotKeyItem&) const;

    private:
        EventHotKeyRef m_ref;
        std::uint32_t m_keycode;
        std::uint32_t m_mask;
    };

    enum EMouseButtonState {
        kMouseButtonUp = 0,
        kMouseButtonDragged,
        kMouseButtonDown,
        kMouseButtonStateMax
    };


    class MouseButtonState {
    public:
        void set(std::uint32_t button, EMouseButtonState state);
        bool any();
        void reset();
        void overwrite(std::uint32_t buttons);

        bool test(std::uint32_t button) const;
        std::int8_t getFirstButtonDown() const;
    private:
        std::bitset<NumButtonIDs> m_buttons;
    };

    typedef std::map<std::uint32_t, HotKeyItem> HotKeyMap;
    typedef std::vector<std::uint32_t> HotKeyIDList;
    typedef std::map<KeyModifierMask, std::uint32_t> ModifierHotKeyMap;
    typedef std::map<HotKeyItem, std::uint32_t> HotKeyToIDMap;

    // true if screen is being used as a primary screen, false otherwise
    bool m_isPrimary;

    // true if mouse has entered the screen
    bool m_isOnScreen;

    // IOKit power management assertion, refreshed on every enter()
    IOPMAssertionID assertionID;

    // the display
    CGDirectDisplayID m_displayID;

    // screen shape stuff
    std::int32_t m_x, m_y;
    std::int32_t m_w, m_h;
    std::int32_t m_xCenter, m_yCenter;

    // mouse state
    mutable std::int32_t m_xCursor, m_yCursor;
    mutable bool m_cursorPosValid;

    /* FIXME: this data structure is explicitly marked mutable due
       to a need to track the state of buttons since the remote
       side only lets us know of change events, and because the
       fakeMouseButton button method is marked 'const'. This is
       Evil, and this should be moved to a place where it need not
       be mutable as soon as possible. */
    mutable MouseButtonState m_buttonState;
    typedef std::map<std::uint16_t, CGEventType> MouseButtonEventMapType;
    std::vector<MouseButtonEventMapType> MouseButtonEventMap;

    bool m_cursorHidden;
    std::int32_t m_dragNumButtonsDown;
    Point m_dragLastPoint;
    EventQueueTimer* m_dragTimer;

    // keyboard stuff
    OSXKeyState* m_keyState;

    // clipboards
    OSXClipboard m_pasteboard;
    std::uint32_t m_sequenceNumber;

    // screen saver stuff
    OSXScreenSaver* m_screensaver;
    bool m_screensaverNotify;

    // clipboard stuff
    bool m_ownClipboard;
    EventQueueTimer* m_clipboardTimer;

    // window object that gets user input events when the server
    // has focus.
    WindowRef m_hiddenWindow;
    // window object that gets user input events when the server
    // does not have focus.
    WindowRef m_userInputWindow;

    // fast user switching
    EventHandlerRef m_switchEventHandlerRef;

    // sleep / wakeup
    std::mutex pm_mutex_;
    Thread* m_pmWatchThread;
    std::condition_variable pm_thread_ready_cv_;
    bool is_pm_thread_ready_ = false;
    CFRunLoopRef m_pmRunloop;
    io_connect_t m_pmRootPort;

    // hot key stuff
    HotKeyMap m_hotKeys;
    HotKeyIDList m_oldHotKeyIDs;
    ModifierHotKeyMap m_modifierHotKeys;
    std::uint32_t m_activeModifierHotKey;
    KeyModifierMask m_activeModifierHotKeyMask;
    HotKeyToIDMap m_hotKeyToIDMap;

    // global hotkey operating mode
    static bool                s_testedForGHOM;
    static bool                s_hasGHOM;

    // Quartz input event support
    CFMachPortRef m_eventTapPort;
    CFRunLoopSourceRef m_eventTapRLSR;

    // for double click coalescing.
    double m_lastClickTime;
    int m_clickState;
    std::int32_t m_lastSingleClickXCursor;
    std::int32_t m_lastSingleClickYCursor;

    // cursor will hide and show on enable and disable if true.
    bool m_autoShowHideCursor;

    IEventQueue* m_events;

    Thread* m_getDropTargetThread;
    std::string m_dropTarget;

#if defined(MAC_OS_X_VERSION_10_7)
    mutable std::mutex carbon_loop_mutex_;
    mutable std::condition_variable cardon_loop_ready_cv_;
    bool is_carbon_loop_ready_ = false;
#endif

    class OSXScreenImpl* m_impl;
};

} // namespace inputleap
