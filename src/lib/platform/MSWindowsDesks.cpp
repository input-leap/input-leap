/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2018 Debauchee Open Source Group
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

#include "platform/MSWindowsDesks.h"

#include "platform/MSWindowsScreen.h"
#include "inputleap/IScreenSaver.h"
#include "inputleap/XScreen.h"
#include "mt/Thread.h"
#include "arch/win32/ArchMiscWindows.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/EventQueueTimer.h"

#include <malloc.h>
#include <VersionHelpers.h>

namespace inputleap {

// these are only defined when WINVER >= 0x0500
#if !defined(SPI_GETMOUSESPEED)
#define SPI_GETMOUSESPEED 112
#endif
#if !defined(SPI_SETMOUSESPEED)
#define SPI_SETMOUSESPEED 113
#endif
#if !defined(SPI_GETSCREENSAVERRUNNING)
#define SPI_GETSCREENSAVERRUNNING 114
#endif

#if !defined(MOUSEEVENTF_HWHEEL)
#define MOUSEEVENTF_HWHEEL 0x1000
#endif

// X button stuff
#if !defined(WM_XBUTTONDOWN)
#define WM_XBUTTONDOWN        0x020B
#define WM_XBUTTONUP        0x020C
#define WM_XBUTTONDBLCLK    0x020D
#define WM_NCXBUTTONDOWN    0x00AB
#define WM_NCXBUTTONUP        0x00AC
#define WM_NCXBUTTONDBLCLK    0x00AD
#define MOUSEEVENTF_XDOWN    0x0080
#define MOUSEEVENTF_XUP        0x0100
#define XBUTTON1            0x0001
#define XBUTTON2            0x0002
#endif
#if !defined(VK_XBUTTON1)
#define VK_XBUTTON1            0x05
#define VK_XBUTTON2            0x06
#endif

// <unused>; <unused>
#define INPUTLEAP_MSG_SWITCH INPUTLEAP_HOOK_LAST_MSG + 1
// <unused>; <unused>
#define INPUTLEAP_MSG_ENTER INPUTLEAP_HOOK_LAST_MSG + 2
// <unused>; <unused>
#define INPUTLEAP_MSG_LEAVE INPUTLEAP_HOOK_LAST_MSG + 3
// wParam = flags, HIBYTE(lParam) = virtual key, LOBYTE(lParam) = scan code
#define INPUTLEAP_MSG_FAKE_KEY INPUTLEAP_HOOK_LAST_MSG + 4
 // flags, XBUTTON id
#define INPUTLEAP_MSG_FAKE_BUTTON INPUTLEAP_HOOK_LAST_MSG + 5
// x; y
#define INPUTLEAP_MSG_FAKE_MOVE INPUTLEAP_HOOK_LAST_MSG + 6
// xDelta; yDelta
#define INPUTLEAP_MSG_FAKE_WHEEL INPUTLEAP_HOOK_LAST_MSG + 7
// POINT*; <unused>
#define INPUTLEAP_MSG_CURSOR_POS INPUTLEAP_HOOK_LAST_MSG + 8
// IKeyState*; <unused>
#define INPUTLEAP_MSG_SYNC_KEYS INPUTLEAP_HOOK_LAST_MSG + 9
// install; <unused>
#define INPUTLEAP_MSG_SCREENSAVER INPUTLEAP_HOOK_LAST_MSG + 10
// dx; dy
#define INPUTLEAP_MSG_FAKE_REL_MOVE INPUTLEAP_HOOK_LAST_MSG + 11
// enable; <unused>
#define INPUTLEAP_MSG_FAKE_INPUT INPUTLEAP_HOOK_LAST_MSG + 12

//
// MSWindowsDesks
//

MSWindowsDesks::MSWindowsDesks(bool isPrimary, bool noHooks,
        const IScreenSaver* screensaver, IEventQueue* events,
        const std::function<void()>& updateKeys, bool stopOnDeskSwitch) :
    m_isPrimary(isPrimary),
    m_noHooks(noHooks),
    m_isOnScreen(m_isPrimary),
    m_x(0), m_y(0),
    m_w(0), m_h(0),
    m_xCenter(0), m_yCenter(0),
    m_multimon(false),
    m_timer(nullptr),
    m_screensaver(screensaver),
    m_screensaverNotify(false),
    m_activeDesk(nullptr),
    m_activeDeskName(),
    m_updateKeys(updateKeys),
    m_events(events),
    m_stopOnDeskSwitch(stopOnDeskSwitch)
{
    m_cursor    = createBlankCursor();
    m_deskClass = createDeskWindowClass(m_isPrimary);
    m_keyLayout = GetKeyboardLayout(GetCurrentThreadId());
    resetOptions();
}

MSWindowsDesks::~MSWindowsDesks()
{
    disable();
    destroyClass(m_deskClass);
    destroyCursor(m_cursor);
}

void
MSWindowsDesks::enable()
{
    m_threadID = GetCurrentThreadId();

    // set the active desk and (re)install the hooks
    checkDesk();

    // install the desk timer.  this timer periodically checks
    // which desk is active and reinstalls the hooks as necessary.
    // we wouldn't need this if windows notified us of a desktop
    // change but as far as i can tell it doesn't.
    m_timer = m_events->newTimer(0.2, nullptr);
    m_events->add_handler(EventType::TIMER, m_timer,
                          [this](const auto& e){ handle_check_desk(); });

    updateKeys();
}

void
MSWindowsDesks::disable()
{
    // remove timer
    if (m_timer != nullptr) {
        m_events->remove_handler(EventType::TIMER, m_timer);
        m_events->deleteTimer(m_timer);
        m_timer = nullptr;
    }

    // destroy desks
    removeDesks();

    m_isOnScreen = m_isPrimary;
}

void
MSWindowsDesks::enter()
{
    sendMessage(INPUTLEAP_MSG_ENTER, 0, 0);
}

void
MSWindowsDesks::leave(HKL keyLayout)
{
    sendMessage(INPUTLEAP_MSG_LEAVE, (WPARAM)keyLayout, 0);
}

void
MSWindowsDesks::resetOptions()
{
    m_leaveForegroundOption = false;
}

void
MSWindowsDesks::setOptions(const OptionsList& options)
{
    for (std::uint32_t i = 0, n = (std::uint32_t)options.size(); i < n; i += 2) {
        if (options[i] == kOptionWin32KeepForeground) {
            m_leaveForegroundOption = (options[i + 1] != 0);
            LOG_DEBUG1("%s the foreground window", m_leaveForegroundOption ? "don\'t grab" : "grab");
        }
    }
}

void
MSWindowsDesks::updateKeys()
{
    sendMessage(INPUTLEAP_MSG_SYNC_KEYS, 0, 0);
}

void MSWindowsDesks::setShape(std::int32_t x, std::int32_t y, std::int32_t width,
                              std::int32_t height, std::int32_t xCenter, std::int32_t yCenter,
                              bool isMultimon)
{
    m_x        = x;
    m_y        = y;
    m_w        = width;
    m_h        = height;
    m_xCenter  = xCenter;
    m_yCenter  = yCenter;
    m_multimon = isMultimon;
}

void
MSWindowsDesks::installScreensaverHooks(bool install)
{
    if (m_isPrimary && m_screensaverNotify != install) {
        m_screensaverNotify = install;
        sendMessage(INPUTLEAP_MSG_SCREENSAVER, install, 0);
    }
}

void
MSWindowsDesks::fakeInputBegin()
{
    sendMessage(INPUTLEAP_MSG_FAKE_INPUT, 1, 0);
}

void
MSWindowsDesks::fakeInputEnd()
{
    sendMessage(INPUTLEAP_MSG_FAKE_INPUT, 0, 0);
}

void MSWindowsDesks::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    POINT pos;
    sendMessage(INPUTLEAP_MSG_CURSOR_POS, reinterpret_cast<WPARAM>(&pos), 0);
    x = pos.x;
    y = pos.y;
}

void
MSWindowsDesks::fakeKeyEvent(
                KeyButton button, UINT virtualKey,
                bool press, bool /*isAutoRepeat*/) const
{
    // synthesize event
    DWORD flags = 0;
    if (((button & 0x100u) != 0)) {
        flags |= KEYEVENTF_EXTENDEDKEY;
    }
    if (!press) {
        flags |= KEYEVENTF_KEYUP;
    }
    sendMessage(INPUTLEAP_MSG_FAKE_KEY, flags,
                            MAKEWORD(static_cast<BYTE>(button & 0xffu),
                                static_cast<BYTE>(virtualKey & 0xffu)));
}

void
MSWindowsDesks::fakeMouseButton(ButtonID button, bool press)
{
    // the system will swap the meaning of left/right for us if
    // the user has configured a left-handed mouse but we don't
    // want it to swap since we want the handedness of the
    // server's mouse.  so pre-swap for a left-handed mouse.
    if (GetSystemMetrics(SM_SWAPBUTTON)) {
        switch (button) {
        case kButtonLeft:
            button = kButtonRight;
            break;

        case kButtonRight:
            button = kButtonLeft;
            break;
        }
    }

    // map button id to button flag and button data
    DWORD data = 0;
    DWORD flags;
    switch (button) {
    case kButtonLeft:
        flags = press ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        break;

    case kButtonMiddle:
        flags = press ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        break;

    case kButtonRight:
        flags = press ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        break;

    case kButtonExtra0:
        data = XBUTTON1;
        flags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
        break;

    case kButtonExtra1:
        data = XBUTTON2;
        flags = press ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
        break;

    default:
        return;
    }

    // do it
    sendMessage(INPUTLEAP_MSG_FAKE_BUTTON, flags, data);
}

void MSWindowsDesks::fakeMouseMove(std::int32_t x, std::int32_t y) const
{
    sendMessage(INPUTLEAP_MSG_FAKE_MOVE,
                            static_cast<WPARAM>(x),
                            static_cast<LPARAM>(y));
}

void MSWindowsDesks::fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
    sendMessage(INPUTLEAP_MSG_FAKE_REL_MOVE,
                            static_cast<WPARAM>(dx),
                            static_cast<LPARAM>(dy));
}

void MSWindowsDesks::fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const
{
    sendMessage(INPUTLEAP_MSG_FAKE_WHEEL, xDelta, yDelta);
}

void
MSWindowsDesks::sendMessage(UINT msg, WPARAM wParam, LPARAM lParam) const
{
    if (m_activeDesk != nullptr && m_activeDesk->m_window != nullptr) {
        PostThreadMessage(m_activeDesk->m_threadID, msg, wParam, lParam);
        waitForDesk();
    }
}

HCURSOR
MSWindowsDesks::createBlankCursor() const
{
    // create a transparent cursor
    int cw = GetSystemMetrics(SM_CXCURSOR);
    int ch = GetSystemMetrics(SM_CYCURSOR);
    std::uint8_t* cursorAND = new std::uint8_t[ch * ((cw + 31) >> 2)];
    std::uint8_t* cursorXOR = new std::uint8_t[ch * ((cw + 31) >> 2)];
    memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
    memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
    HCURSOR c = CreateCursor(MSWindowsScreen::getWindowInstance(),
                            0, 0, cw, ch, cursorAND, cursorXOR);
    delete[] cursorXOR;
    delete[] cursorAND;
    return c;
}

void
MSWindowsDesks::destroyCursor(HCURSOR cursor) const
{
    if (cursor != nullptr) {
        DestroyCursor(cursor);
    }
}

ATOM
MSWindowsDesks::createDeskWindowClass(bool isPrimary) const
{
    WNDCLASSEX classInfo;
    classInfo.cbSize        = sizeof(classInfo);
    classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
    classInfo.lpfnWndProc   = isPrimary ?
                                &MSWindowsDesks::primaryDeskProc :
                                &MSWindowsDesks::secondaryDeskProc;
    classInfo.cbClsExtra    = 0;
    classInfo.cbWndExtra    = 0;
    classInfo.hInstance     = MSWindowsScreen::getWindowInstance();
    classInfo.hIcon = nullptr;
    classInfo.hCursor       = m_cursor;
    classInfo.hbrBackground = nullptr;
    classInfo.lpszMenuName = nullptr;
    classInfo.lpszClassName = "InputLeapDesk";
    classInfo.hIconSm = nullptr;
    return RegisterClassEx(&classInfo);
}

void
MSWindowsDesks::destroyClass(ATOM windowClass) const
{
    if (windowClass != 0) {
        UnregisterClass(MAKEINTATOM(windowClass),
                            MSWindowsScreen::getWindowInstance());
    }
}

HWND
MSWindowsDesks::createWindow(ATOM windowClass, const char* name) const
{
    HWND window = CreateWindowEx(WS_EX_TRANSPARENT |
                                    WS_EX_TOOLWINDOW,
                                MAKEINTATOM(windowClass),
                                name,
                                WS_POPUP,
                                0, 0, 1, 1,
                                nullptr, nullptr,
                                MSWindowsScreen::getWindowInstance(),
                                nullptr);
    if (window == nullptr) {
        LOG_ERR("failed to create window: %d", GetLastError());
        throw XScreenOpenFailure();
    }
    return window;
}

void
MSWindowsDesks::destroyWindow(HWND hwnd) const
{
    if (hwnd != nullptr) {
        DestroyWindow(hwnd);
    }
}

LRESULT CALLBACK
MSWindowsDesks::primaryDeskProc(
                HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
MSWindowsDesks::secondaryDeskProc(
                HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // would like to detect any local user input and hide the hider
    // window but for now we just detect mouse motion.
    bool hide = false;
    switch (msg) {
    case WM_MOUSEMOVE:
        if (LOWORD(lParam) != 0 || HIWORD(lParam) != 0) {
            hide = true;
        }
        break;
    }

    if (hide && IsWindowVisible(hwnd)) {
        ReleaseCapture();
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE |
                            SWP_NOACTIVATE | SWP_HIDEWINDOW);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void MSWindowsDesks::deskMouseMove(std::int32_t x, std::int32_t y) const
{
    // when using absolute positioning with mouse_event(),
    // the normalized device coordinates range over only
    // the primary screen.
    std::int32_t w = GetSystemMetrics(SM_CXSCREEN);
    std::int32_t h = GetSystemMetrics(SM_CYSCREEN);
    mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                            (DWORD)((65535.0f * x) / (w - 1) + 0.5f),
                            (DWORD)((65535.0f * y) / (h - 1) + 0.5f),
                            0, 0);
}

void MSWindowsDesks::deskMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
    // relative moves are subject to cursor acceleration which we don't
    // want.so we disable acceleration, do the relative move, then
    // restore acceleration.  there's a slight chance we'll end up in
    // the wrong place if the user moves the cursor using this system's
    // mouse while simultaneously moving the mouse on the server
    // system.  that defeats the purpose of InputLeap so we'll assume
    // that won't happen.  even if it does, the next mouse move will
    // correct the position.

    // save mouse speed & acceleration
    int oldSpeed[4];
    bool accelChanged =
                SystemParametersInfo(SPI_GETMOUSE,0, oldSpeed, 0) &&
                SystemParametersInfo(SPI_GETMOUSESPEED, 0, oldSpeed + 3, 0);

    // use 1:1 motion
    if (accelChanged) {
        int newSpeed[4] = { 0, 0, 0, 1 };
        accelChanged =
                SystemParametersInfo(SPI_SETMOUSE, 0, newSpeed, 0) ||
                SystemParametersInfo(SPI_SETMOUSESPEED, 0, newSpeed + 3, 0);
    }

    // move relative to mouse position
    mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);

    // restore mouse speed & acceleration
    if (accelChanged) {
        SystemParametersInfo(SPI_SETMOUSE, 0, oldSpeed, 0);
        SystemParametersInfo(SPI_SETMOUSESPEED, 0, oldSpeed + 3, 0);
    }
}

void
MSWindowsDesks::deskEnter(Desk* desk)
{
    if (!m_isPrimary) {
        ReleaseCapture();
    }
    ShowCursor(TRUE);
    SetWindowPos(desk->m_window, HWND_BOTTOM, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE |
                            SWP_NOACTIVATE | SWP_HIDEWINDOW);

    // restore the foreground window
    // XXX -- this raises the window to the top of the Z-order.  we
    // want it to stay wherever it was to properly support X-mouse
    // (mouse over activation) but i've no idea how to do that.
    // the obvious workaround of using SetWindowPos() to move it back
    // after being raised doesn't work.
    DWORD thisThread =
        GetWindowThreadProcessId(desk->m_window, nullptr);
    DWORD thatThread =
        GetWindowThreadProcessId(desk->m_foregroundWindow, nullptr);
    AttachThreadInput(thatThread, thisThread, TRUE);
    SetForegroundWindow(desk->m_foregroundWindow);
    AttachThreadInput(thatThread, thisThread, FALSE);
    EnableWindow(desk->m_window, FALSE);
    desk->m_foregroundWindow = nullptr;
}

void
MSWindowsDesks::deskLeave(Desk* desk, HKL keyLayout)
{
    ShowCursor(FALSE);
    if (m_isPrimary) {
        // map a window to hide the cursor and to use whatever keyboard
        // layout we choose rather than the keyboard layout of the last
        // active window.
        int x, y, w, h;
        // with a low level hook the cursor will never budge so
        // just a 1x1 window is sufficient.
        x = m_xCenter;
        y = m_yCenter;
        w = 1;
        h = 1;
        SetWindowPos(desk->m_window, HWND_TOP, x, y, w, h,
                            SWP_NOACTIVATE | SWP_SHOWWINDOW);

        // since we're using low-level hooks, disable the foreground window
        // so it can't mess up any of our keyboard events.  the console
        // program, for example, will cause characters to be reported as
        // unshifted, regardless of the shift key state.  interestingly
        // we do see the shift key go down and up.
        //
        // note that we must enable the window to activate it and we
        // need to disable the window on deskEnter.
        desk->m_foregroundWindow = getForegroundWindow();
        if (desk->m_foregroundWindow != nullptr) {
            EnableWindow(desk->m_window, TRUE);
            SetActiveWindow(desk->m_window);
            DWORD thisThread =
                GetWindowThreadProcessId(desk->m_window, nullptr);
            DWORD thatThread =
                GetWindowThreadProcessId(desk->m_foregroundWindow, nullptr);
            AttachThreadInput(thatThread, thisThread, TRUE);
            SetForegroundWindow(desk->m_window);
            AttachThreadInput(thatThread, thisThread, FALSE);
        }

        // switch to requested keyboard layout
        ActivateKeyboardLayout(keyLayout, 0);
    }
    else {
        // move hider window under the cursor center, raise, and show it
        SetWindowPos(desk->m_window, HWND_TOP,
                            m_xCenter, m_yCenter, 1, 1,
                            SWP_NOACTIVATE | SWP_SHOWWINDOW);

        // watch for mouse motion.  if we see any then we hide the
        // hider window so the user can use the physically attached
        // mouse if desired.  we'd rather not capture the mouse but
        // we aren't notified when the mouse leaves our window.
        SetCapture(desk->m_window);

        // warp the mouse to the cursor center
        LOG_DEBUG2("warping cursor to center: %+d,%+d", m_xCenter, m_yCenter);
        deskMouseMove(m_xCenter, m_yCenter);
    }
}

void MSWindowsDesks::desk_thread(Desk* desk)
{
    MSG msg;

    // use given desktop for this thread
    desk->m_threadID         = GetCurrentThreadId();
    desk->m_window = nullptr;
    desk->m_foregroundWindow = nullptr;
    if (desk->m_desk != nullptr && SetThreadDesktop(desk->m_desk) != 0) {
        // create a message queue
        PeekMessage(&msg, nullptr, 0,0, PM_NOREMOVE);

        // create a window.  we use this window to hide the cursor.
        try {
            desk->m_window = createWindow(m_deskClass, "InputLeapDesk");
            LOG_DEBUG("desk %s window is 0x%08x", desk->m_name.c_str(), desk->m_window);
        }
        catch (...) {
            // ignore
            LOG_DEBUG("can't create desk window for %s", desk->m_name.c_str());
        }
    }

    // tell main thread that we're ready
    {
        std::lock_guard<std::mutex> lock(mutex_);
        is_desks_ready_ = true;
        desks_ready_cv_.notify_all();
    }

    while (GetMessage(&msg, nullptr, 0, 0)) {
        switch (msg.message) {
        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;

        case INPUTLEAP_MSG_SWITCH:
            if (m_isPrimary && !m_noHooks) {
                MSWindowsHook::uninstall();
                if (m_screensaverNotify) {
                    MSWindowsHook::uninstallScreenSaver();
                    MSWindowsHook::installScreenSaver();
                }
                if (!MSWindowsHook::install()) {
                    // we won't work on this desk
                    LOG_DEBUG("Cannot hook on this desk");
                }
                // a window on the primary screen with low-level hooks
                // should never activate.
                if (desk->m_window)
                    EnableWindow(desk->m_window, FALSE);
            }
            break;

        case INPUTLEAP_MSG_ENTER:
            m_isOnScreen = true;
            deskEnter(desk);
            break;

        case INPUTLEAP_MSG_LEAVE:
            m_isOnScreen = false;
            m_keyLayout  = (HKL)msg.wParam;
            deskLeave(desk, m_keyLayout);
            break;

        case INPUTLEAP_MSG_FAKE_KEY:
            keybd_event(HIBYTE(msg.lParam), LOBYTE(msg.lParam), (DWORD)msg.wParam, 0);
            break;

        case INPUTLEAP_MSG_FAKE_BUTTON:
            if (msg.wParam != 0) {
                mouse_event((DWORD)msg.wParam, 0, 0, (DWORD)msg.lParam, 0);
            }
            break;

        case INPUTLEAP_MSG_FAKE_MOVE:
            deskMouseMove(static_cast<std::int32_t>(msg.wParam),
                          static_cast<std::int32_t>(msg.lParam));
            break;

        case INPUTLEAP_MSG_FAKE_REL_MOVE:
            deskMouseRelativeMove(static_cast<std::int32_t>(msg.wParam),
                                  static_cast<std::int32_t>(msg.lParam));
            break;

        case INPUTLEAP_MSG_FAKE_WHEEL:
            if (msg.lParam != 0) {
                mouse_event(MOUSEEVENTF_WHEEL, 0, 0, (DWORD)msg.lParam, 0);
            }
            else if (IsWindowsVistaOrGreater() && msg.wParam != 0) {
                mouse_event(MOUSEEVENTF_HWHEEL, 0, 0, (DWORD)msg.wParam, 0);
            }
            break;

        case INPUTLEAP_MSG_CURSOR_POS: {
            POINT* pos = reinterpret_cast<POINT*>(msg.wParam);
            if (!GetCursorPos(pos)) {
                pos->x = m_xCenter;
                pos->y = m_yCenter;
            }
            break;
        }

        case INPUTLEAP_MSG_SYNC_KEYS:
            m_updateKeys();
            break;

        case INPUTLEAP_MSG_SCREENSAVER:
            if (!m_noHooks) {
                if (msg.wParam != 0) {
                    MSWindowsHook::installScreenSaver();
                }
                else {
                    MSWindowsHook::uninstallScreenSaver();
                }
            }
            break;

        case INPUTLEAP_MSG_FAKE_INPUT:
            keybd_event(INPUTLEAP_HOOK_FAKE_INPUT_VIRTUAL_KEY,
                        INPUTLEAP_HOOK_FAKE_INPUT_SCANCODE,
                        msg.wParam ? 0 : KEYEVENTF_KEYUP, 0);
            break;
        }

        // notify that message was processed
        std::lock_guard<std::mutex> lock(mutex_);
        is_desks_ready_ = true;
        desks_ready_cv_.notify_all();
    }

    // clean up
    deskEnter(desk);
    if (desk->m_window != nullptr) {
        DestroyWindow(desk->m_window);
    }
    if (desk->m_desk != nullptr) {
        closeDesktop(desk->m_desk);
    }
}

MSWindowsDesks::Desk* MSWindowsDesks::addDesk(const std::string& name, HDESK hdesk)
{
    Desk* desk      = new Desk;
    desk->m_name     = name;
    desk->m_desk     = hdesk;
    desk->m_targetID = GetCurrentThreadId();
    desk->m_thread   = new Thread([this, desk]() { desk_thread(desk); });
    waitForDesk();
    m_desks.insert(std::make_pair(name, desk));
    return desk;
}

void
MSWindowsDesks::removeDesks()
{
    for (Desks::iterator index = m_desks.begin();
                            index != m_desks.end(); ++index) {
        Desk* desk = index->second;
        PostThreadMessage(desk->m_threadID, WM_QUIT, 0, 0);
        desk->m_thread->wait();
        delete desk->m_thread;
        delete desk;
    }
    m_desks.clear();
    m_activeDesk  = nullptr;
    m_activeDeskName = "";
}

void
MSWindowsDesks::checkDesk()
{
    // get current desktop.  if we already know about it then return.
    Desk* desk;
    HDESK hdesk  = openInputDesktop();
    std::string name = getDesktopName(hdesk);
    Desks::const_iterator index = m_desks.find(name);
    if (index == m_desks.end()) {
        desk = addDesk(name, hdesk);
        // hold on to hdesk until thread exits so the desk can't
        // be removed by the system
    }
    else {
        closeDesktop(hdesk);
        desk = index->second;
    }

    // if we are told to shut down on desk switch, and this is not the
    // first switch, then shut down.
    if (m_stopOnDeskSwitch && m_activeDesk != nullptr && name != m_activeDeskName) {
        LOG_DEBUG("shutting down because of desk switch to \"%s\"", name.c_str());
        m_events->add_event(EventType::QUIT);
        return;
    }

    // if active desktop changed then tell the old and new desk threads
    // about the change.  don't switch desktops when the screensaver is
    // active because we'd most likely switch to the screensaver desktop
    // which would have the side effect of forcing the screensaver to
    // stop.
    if (name != m_activeDeskName && !m_screensaver->isActive()) {
        // show cursor on previous desk
        bool wasOnScreen = m_isOnScreen;
        if (!wasOnScreen) {
            sendMessage(INPUTLEAP_MSG_ENTER, 0, 0);
        }

        // check for desk accessibility change.  we don't get events
        // from an inaccessible desktop so when we switch from an
        // inaccessible desktop to an accessible one we have to
        // update the keyboard state.
        LOG_DEBUG("switched to desk \"%s\"", name.c_str());
        bool syncKeys = false;
        bool isAccessible = isDeskAccessible(desk);
        if (isDeskAccessible(m_activeDesk) != isAccessible) {
            if (isAccessible) {
                LOG_DEBUG("desktop is now accessible");
                syncKeys = true;
            }
            else {
                LOG_DEBUG("desktop is now inaccessible");
            }
        }

        // switch desk
        m_activeDesk     = desk;
        m_activeDeskName = name;
        sendMessage(INPUTLEAP_MSG_SWITCH, 0, 0);

        // hide cursor on new desk
        if (!wasOnScreen) {
            sendMessage(INPUTLEAP_MSG_LEAVE, (WPARAM)m_keyLayout, 0);
        }

        // update keys if necessary
        if (syncKeys) {
            updateKeys();
        }
    }
    else if (name != m_activeDeskName) {
        // screen saver might have started
        PostThreadMessage(m_threadID, INPUTLEAP_MSG_SCREEN_SAVER, TRUE, 0);
    }
}

bool
MSWindowsDesks::isDeskAccessible(const Desk* desk) const
{
    return (desk != nullptr && desk->m_desk != nullptr);
}

void
MSWindowsDesks::waitForDesk() const
{
    MSWindowsDesks* self = const_cast<MSWindowsDesks*>(this);

    std::unique_lock<std::mutex> lock(mutex_);
    desks_ready_cv_.wait(lock, [this](){ return is_desks_ready_; });
    self->is_desks_ready_ = false;
}

void MSWindowsDesks::handle_check_desk()
{
    checkDesk();

    // also check if screen saver is running if on a modern OS and
    // this is the primary screen.
    if (m_isPrimary) {
        BOOL running;
        SystemParametersInfo(SPI_GETSCREENSAVERRUNNING, 0, &running, FALSE);
        PostThreadMessage(m_threadID, INPUTLEAP_MSG_SCREEN_SAVER, running, 0);
    }
}

HDESK
MSWindowsDesks::openInputDesktop()
{
    return OpenInputDesktop(
        DF_ALLOWOTHERACCOUNTHOOK, TRUE,
        DESKTOP_CREATEWINDOW | DESKTOP_HOOKCONTROL | GENERIC_WRITE);
}

void
MSWindowsDesks::closeDesktop(HDESK desk)
{
    if (desk != nullptr) {
        CloseDesktop(desk);
    }
}

std::string MSWindowsDesks::getDesktopName(HDESK desk)
{
    if (desk == nullptr) {
        return {};
    }
    else {
        DWORD size;
        GetUserObjectInformation(desk, UOI_NAME, nullptr, 0, &size);
        TCHAR* name = (TCHAR*)alloca(size + sizeof(TCHAR));
        GetUserObjectInformation(desk, UOI_NAME, name, size, &size);
        std::string result(name);
        return result;
    }
}

HWND
MSWindowsDesks::getForegroundWindow() const
{
    // Ideally we'd return nullptr as much as possible, only returning
    // the actual foreground window when we know it's going to mess
    // up our keyboard input.  For now we'll just let the user
    // decide.
    if (m_leaveForegroundOption) {
        return nullptr;
    }
    return GetForegroundWindow();
}

} // namespace inputleap
