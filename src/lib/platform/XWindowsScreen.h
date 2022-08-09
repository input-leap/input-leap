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

#pragma once

#include "config.h"

#include "inputleap/PlatformScreen.h"
#include "inputleap/KeyMap.h"
#include "common/stdset.h"
#include "common/stdvector.h"
#include "XWindowsImpl.h"

#include <X11/Xlib.h>

class XWindowsClipboard;
class XWindowsKeyState;
class XWindowsScreenSaver;

//! Implementation of IPlatformScreen for X11
class XWindowsScreen : public PlatformScreen {
public:
    XWindowsScreen(IXWindowsImpl* impl, const char* displayName, bool isPrimary,
        bool disableXInitThreads, int mouseScrollDelta,
        IEventQueue* events);
    ~XWindowsScreen() override;

    //! @name manipulators
    //@{

    //@}

    // IScreen overrides
    void* getEventTarget() const override;
    bool getClipboard(ClipboardID id, IClipboard*) const override;
    void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width, std::int32_t& height) const override;
    void getCursorPos(std::int32_t& x, std::int32_t& y) const override;

    // IPrimaryScreen overrides
    void reconfigure(std::uint32_t activeSides) override;
    void warpCursor(std::int32_t x, std::int32_t y) override;
    std::uint32_t registerHotKey(KeyID key, KeyModifierMask mask) override;
    void unregisterHotKey(std::uint32_t id) override;
    void fakeInputBegin() override;
    void fakeInputEnd() override;
    std::int32_t getJumpZoneSize() const override;
    bool isAnyMouseButtonDown(std::uint32_t& buttonID) const override;
    void getCursorCenter(std::int32_t& x, std::int32_t& y) const override;

    // ISecondaryScreen overrides
    void fakeMouseButton(ButtonID id, bool press) override;
    void fakeMouseMove(std::int32_t x, std::int32_t y) override;
    void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const override;
    void fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const override;

    // IPlatformScreen overrides
    void enable() override;
    void disable() override;
    void enter() override;
    bool leave() override;
    bool setClipboard(ClipboardID, const IClipboard*) override;
    void checkClipboards() override;
    void openScreensaver(bool notify) override;
    void closeScreensaver() override;
    void screensaver(bool activate) override;
    void resetOptions() override;
    void setOptions(const OptionsList& options) override;
    void setSequenceNumber(std::uint32_t) override;
    bool isPrimary() const override;

protected:
    // IPlatformScreen overrides
    void handleSystemEvent(const Event&, void*) override;
    void updateButtons() override;
    IKeyState* getKeyState() const override;

private:
    // event sending
    void sendEvent(Event::Type, void* = nullptr);
    void                sendClipboardEvent(Event::Type, ClipboardID);

    // create the transparent cursor
    Cursor              createBlankCursor() const;

    // determine the clipboard from the X selection.  returns
    // kClipboardEnd if no such clipboard.
    ClipboardID         getClipboardID(Atom selection) const;

    // continue processing a selection request
    void                processClipboardRequest(Window window,
                            Time time, Atom property);

    // terminate a selection request
    void                destroyClipboardRequest(Window window);

    // X I/O error handler
    void                onError();
    static int            ioErrorHandler(Display*);

private:
    class KeyEventFilter {
    public:
        int m_event;
        Window m_window;
        Time m_time;
        KeyCode m_keycode;
    };

    Display*            openDisplay(const char* displayName);
    void                saveShape();
    Window              openWindow() const;
    void                openIM();

    bool                grabMouseAndKeyboard();
    void                onKeyPress(XKeyEvent&);
    void                onKeyRelease(XKeyEvent&, bool isRepeat);
    bool                onHotKey(XKeyEvent&, bool isRepeat);
    void                onMousePress(const XButtonEvent&);
    void                onMouseRelease(const XButtonEvent&);
    void                onMouseMove(const XMotionEvent&);

    // Returns the number of scroll events needed after the current delta has
    // been taken into account
    int x_accumulateMouseScroll(std::int32_t xDelta) const;
    int y_accumulateMouseScroll(std::int32_t yDelta) const;

    bool                detectXI2();
    void                selectXIRawMotion();
    void                selectEvents(Window) const;
    void                doSelectEvents(Window) const;

    KeyID               mapKeyFromX(XKeyEvent*) const;
    ButtonID            mapButtonFromX(const XButtonEvent*) const;
    unsigned int        mapButtonToX(ButtonID id) const;

    void warpCursorNoFlush(std::int32_t x, std::int32_t y);

    void                refreshKeyboard(XEvent*);

    static Bool         findKeyEvent(Display*, XEvent* xevent, XPointer arg);

private:
    struct HotKeyItem {
    public:
        HotKeyItem(int, unsigned int);

        bool            operator<(const HotKeyItem&) const;

    private:
        int m_keycode;
        unsigned int m_mask;
    };
    typedef std::set<bool> FilteredKeycodes;
    typedef std::vector<std::pair<int, unsigned int> > HotKeyList;
    typedef std::map<std::uint32_t, HotKeyList> HotKeyMap;
    typedef std::vector<std::uint32_t> HotKeyIDList;
    typedef std::map<HotKeyItem, std::uint32_t> HotKeyToIDMap;

    IXWindowsImpl* m_impl;

    // true if screen is being used as a primary screen, false otherwise
    bool m_isPrimary;

    // The size of a smallest supported scroll event, in points
    int m_mouseScrollDelta;

    // Accumulates scrolls of less than m_?_mouseScrollDelta across multiple
    // scroll events. We dispatch a scroll event whenever the accumulated scroll
    // becomes larger than m_?_mouseScrollDelta
    mutable int m_x_accumulatedScroll;
    mutable int m_y_accumulatedScroll;

    Display* m_display;
    Window m_root;
    Window m_window;

    // true if mouse has entered the screen
    bool m_isOnScreen;

    // screen shape stuff
    std::int32_t m_x, m_y;
    std::int32_t m_w, m_h;
    std::int32_t m_xCenter, m_yCenter;

    // last mouse position
    std::int32_t m_xCursor, m_yCursor;

    // keyboard stuff
    XWindowsKeyState* m_keyState;

    // hot key stuff
    HotKeyMap m_hotKeys;
    HotKeyIDList m_oldHotKeyIDs;
    HotKeyToIDMap m_hotKeyToIDMap;

    // input focus stuff
    Window m_lastFocus;
    int m_lastFocusRevert;

    // input method stuff
    XIM m_im;
    XIC m_ic;
    KeyCode m_lastKeycode;
    FilteredKeycodes m_filtered;

    // clipboards
    XWindowsClipboard* m_clipboard[kClipboardEnd];
    std::uint32_t m_sequenceNumber;

    // screen saver stuff
    XWindowsScreenSaver* m_screensaver;
    bool m_screensaverNotify;

    // logical to physical button mapping.  m_buttons[i] gives the
    // physical button for logical button i+1.
    std::vector<unsigned char> m_buttons;

    // true if global auto-repeat was enabled before we turned it off
    bool m_autoRepeat;

    // stuff to workaround xtest being xinerama unaware.  attempting
    // to fake a mouse motion under xinerama may behave strangely,
    // especially if screen 0 is not at 0,0 or if faking a motion on
    // a screen other than screen 0.
    bool m_xtestIsXineramaUnaware;
    bool m_xinerama;

    // stuff to work around lost focus issues on certain systems
    // (ie: a MythTV front-end).
    bool m_preserveFocus;

    // XKB extension stuff
    bool m_xkb;
    int m_xkbEventBase;

    bool m_xi2detected;

    // XRandR extension stuff
    bool m_xrandr;
    int m_xrandrEventBase;

    IEventQueue* m_events;
    inputleap::KeyMap m_keyMap;

    // pointer to (singleton) screen.  this is only needed by
    // ioErrorHandler().
    static XWindowsScreen*    s_screen;
};
