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

#pragma once

#include "platform/MSWindowsHook.h"
#include "inputleap/PlatformScreen.h"
#include "inputleap/DragInformation.h"
#include "platform/synwinhk.h"
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class EventQueueTimer;
class MSWindowsDesks;
class MSWindowsKeyState;
class MSWindowsScreenSaver;
class Thread;
class MSWindowsDropTarget;

//! Implementation of IPlatformScreen for Microsoft Windows
class MSWindowsScreen : public PlatformScreen {
public:
    MSWindowsScreen(
        bool isPrimary,
        bool noHooks,
        bool stopOnDeskSwitch,
        IEventQueue* events);
    virtual ~MSWindowsScreen();

    //! @name manipulators
    //@{

    //! Initialize
    /*!
    Saves the application's HINSTANCE.  This \b must be called by
    WinMain with the HINSTANCE it was passed.
    */
    static void init(HINSTANCE);

    //@}
    //! @name accessors
    //@{

    //! Get instance
    /*!
    Returns the application instance handle passed to init().
    */
    static HINSTANCE getWindowInstance();

    //@}

    // IScreen overrides
    virtual void* getEventTarget() const;
    virtual bool getClipboard(ClipboardID id, IClipboard*) const;
    virtual void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                          std::int32_t& height) const;
    virtual void getCursorPos(std::int32_t& x, std::int32_t& y) const;

    // IPrimaryScreen overrides
    virtual void reconfigure(std::uint32_t activeSides);
    virtual void warpCursor(std::int32_t x, std::int32_t y);
    virtual std::uint32_t registerHotKey(KeyID key,
                            KeyModifierMask mask);
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

    // IKeyState overrides
    virtual void updateKeys();
    virtual void fakeKeyDown(KeyID id, KeyModifierMask mask,
                            KeyButton button);
    virtual bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                               KeyButton button);
    virtual bool fakeKeyUp(KeyButton button);
    virtual void fakeAllKeysUp();

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
    virtual const std::string& getDropTarget() const;
    virtual void setDropTarget(const std::string&);

protected:
    // IPlatformScreen overrides
    virtual void handleSystemEvent(const Event&, void*);
    virtual void updateButtons();
    virtual IKeyState* getKeyState() const;

    // simulate a local key to the system directly
    void fakeLocalKey(KeyButton button, bool press) const;

private:
    // initialization and shutdown operations
    HCURSOR createBlankCursor() const;
    void destroyCursor(HCURSOR cursor) const;
    ATOM createWindowClass() const;
    ATOM createDeskWindowClass(bool isPrimary) const;
    void destroyClass(ATOM windowClass) const;
    HWND createWindow(ATOM windowClass, const char* name) const;
    HWND createDropWindow(ATOM windowClass, const char* name) const;
    void destroyWindow(HWND) const;

    // convenience function to send events
public: // HACK
    void sendEvent(Event::Type type, void* = nullptr);
private: // HACK
    void sendClipboardEvent(Event::Type type, ClipboardID id);

    // handle message before it gets dispatched.  returns true iff
    // the message should not be dispatched.
    bool onPreDispatch(HWND, UINT, WPARAM, LPARAM);

    // handle message before it gets dispatched.  returns true iff
    // the message should not be dispatched.
    bool onPreDispatchPrimary(HWND, UINT, WPARAM, LPARAM);

    // handle message.  returns true iff handled and optionally sets
    // \c *result (which defaults to 0).
    bool onEvent(HWND, UINT, WPARAM, LPARAM, LRESULT* result);

    // message handlers
    bool onMark(std::uint32_t mark);
    bool onKey(WPARAM, LPARAM);
    bool onHotKey(WPARAM, LPARAM);
    bool onMouseButton(WPARAM, LPARAM);
    bool onMouseMove(std::int32_t x, std::int32_t y);
    bool onMouseWheel(std::int32_t xDelta, std::int32_t yDelta);
    bool onScreensaver(bool activated);
    bool onDisplayChange();
    bool onClipboardChange();

    // warp cursor without discarding queued events
    void warpCursorNoFlush(std::int32_t x, std::int32_t y);

    // discard posted messages
    void nextMark();

    // test if event should be ignored
    bool ignore() const;

    // update screen size cache
    void updateScreenShape();

    // fix timer callback
    void handleFixes(const Event&, void*);

    // fix the clipboard viewer chain
    void fixClipboardViewer();

    // enable/disable special key combinations so we can catch/pass them
    void enableSpecialKeys(bool) const;

    // map a button event to a button ID
    ButtonID mapButtonFromEvent(WPARAM msg, LPARAM button) const;

    // map a button event to a press (true) or release (false)
    bool mapPressFromEvent(WPARAM msg, LPARAM button) const;

    // job to update the key state
    void updateKeysCB();

    // determine whether the mouse is hidden by the system and force
    // it to be displayed if user has entered this secondary screen.
    void forceShowCursor();

    // forceShowCursor uses MouseKeys to show the cursor.  since we
    // don't actually want MouseKeys behavior we have to make sure
    // it applies when NumLock is in whatever state it's not in now.
    // this method does that.
    void updateForceShowCursor();

    // our window proc
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

    // save last position of mouse to compute next delta movement
    void saveMousePosition(std::int32_t x, std::int32_t y);

    // check if it is a modifier key repeating message
    bool isModifierRepeat(KeyModifierMask oldState,
                            KeyModifierMask state, WPARAM wParam) const;

    // send drag info and data back to server
    void send_drag_thread();

private:
    struct HotKeyItem {
    public:
        HotKeyItem(UINT vk, UINT modifiers);

        UINT getVirtualKey() const;

        bool            operator<(const HotKeyItem&) const;

    private:
        UINT m_keycode;
        UINT m_mask;
    };
    typedef std::map<std::uint32_t, HotKeyItem> HotKeyMap;
    typedef std::vector<std::uint32_t> HotKeyIDList;
    typedef std::map<HotKeyItem, std::uint32_t> HotKeyToIDMap;
    typedef std::vector<KeyButton> PrimaryKeyDownList;

    static HINSTANCE    s_windowInstance;

    // true if screen is being used as a primary screen, false otherwise
    bool m_isPrimary;

    // true if hooks are not to be installed (useful for debugging)
    bool m_noHooks;

    // true if mouse has entered the screen
    bool m_isOnScreen;

    // our resources
    ATOM m_class;

    // screen shape stuff
    std::int32_t m_x, m_y;
    std::int32_t m_w, m_h;
    std::int32_t m_xCenter, m_yCenter;

    // true if system appears to have multiple monitors
    bool m_multimon;

    // last mouse position
    std::int32_t m_xCursor, m_yCursor;

    // last clipboard
    std::uint32_t m_sequenceNumber;

    // used to discard queued messages that are no longer needed
    std::uint32_t m_mark;
    std::uint32_t m_markReceived;

    // the main loop's thread id
    DWORD m_threadID;

    // timer for periodically checking stuff that requires polling
    EventQueueTimer* m_fixTimer;

    // the keyboard layout to use when off primary screen
    HKL m_keyLayout;

    // screen saver stuff
    MSWindowsScreenSaver* m_screensaver;
    bool m_screensaverNotify;
    bool m_screensaverActive;

    // clipboard stuff.  our window is used mainly as a clipboard
    // owner and as a link in the clipboard viewer chain.
    HWND m_window;
    HWND m_nextClipboardWindow;
    bool m_ownClipboard;

    // one desk per desktop and a cond var to communicate with it
    MSWindowsDesks* m_desks;

    // keyboard stuff
    MSWindowsKeyState* m_keyState;

    // hot key stuff
    HotKeyMap m_hotKeys;
    HotKeyIDList m_oldHotKeyIDs;
    HotKeyToIDMap m_hotKeyToIDMap;

    // map of button state
    bool m_buttons[NumButtonIDs];

    // the system shows the mouse cursor when an internal display count
    // is >= 0.  this count is maintained per application but there's
    // apparently a system wide count added to the application's count.
    // this system count is 0 if there's a mouse attached to the system
    // and -1 otherwise.  the MouseKeys accessibility feature can modify
    // this system count by making the system appear to have a mouse.
    //
    // m_hasMouse is true iff there's a mouse attached to the system or
    // MouseKeys is simulating one.  we track this so we can force the
    // cursor to be displayed when the user has entered this screen.
    // m_showingMouse is true when we're doing that.
    bool m_hasMouse;
    bool m_showingMouse;
    bool m_gotOldMouseKeys;
    MOUSEKEYS m_mouseKeys;
    MOUSEKEYS m_oldMouseKeys;

    MSWindowsHook m_hook;

    static MSWindowsScreen*
                        s_screen;

    IEventQueue* m_events;

    mutable std::string m_dropTargetPath;

    MSWindowsDropTarget* m_dropTarget;
    HWND m_dropWindow;
    const int m_dropWindowSize;

    Thread* m_sendDragThread;

    PrimaryKeyDownList m_primaryKeyDownList;
};
