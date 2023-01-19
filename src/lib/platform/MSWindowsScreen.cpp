/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2018 Debauchee Open Source Group
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

#include "platform/MSWindowsScreen.h"

#include "platform/MSWindowsDropTarget.h"
#include "client/Client.h"
#include "platform/MSWindowsClipboard.h"
#include "platform/MSWindowsDesks.h"
#include "platform/MSWindowsEventQueueBuffer.h"
#include "platform/MSWindowsKeyState.h"
#include "platform/MSWindowsScreenSaver.h"
#include "inputleap/Clipboard.h"
#include "inputleap/KeyMap.h"
#include "inputleap/XScreen.h"
#include "inputleap/App.h"
#include "inputleap/ArgsBase.h"
#include "inputleap/ClientApp.h"
#include "mt/Thread.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/Time.h"

#include <string.h>
#include <Shlobj.h>
#include <comutil.h>
#include <algorithm>

//
// add backwards compatible multihead support (and suppress bogus warning).
// this isn't supported on MinGW yet AFAICT.
//
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4706) // assignment within conditional
#pragma warning(disable: 4996) // GetVersionExA was declared deprecated
#define COMPILE_MULTIMON_STUBS
#include <multimon.h>
#pragma warning(pop)
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

// WM_POWERBROADCAST stuff
#if !defined(PBT_APMRESUMEAUTOMATIC)
#define PBT_APMRESUMEAUTOMATIC    0x0012
#endif

namespace inputleap {

HINSTANCE MSWindowsScreen::s_windowInstance = nullptr;
MSWindowsScreen* MSWindowsScreen::s_screen  = nullptr;

MSWindowsScreen::MSWindowsScreen(
    bool isPrimary,
    bool noHooks,
    bool stopOnDeskSwitch,
    IEventQueue* events) :
    PlatformScreen(events),
    m_isPrimary(isPrimary),
    m_noHooks(noHooks),
    m_isOnScreen(m_isPrimary),
    m_class(0),
    m_x(0), m_y(0),
    m_w(0), m_h(0),
    m_xCenter(0), m_yCenter(0),
    m_multimon(false),
    m_xCursor(0), m_yCursor(0),
    m_sequenceNumber(0),
    m_mark(0),
    m_markReceived(0),
    m_fixTimer(nullptr),
    m_keyLayout(nullptr),
    m_screensaver(nullptr),
    m_screensaverNotify(false),
    m_screensaverActive(false),
    m_window(nullptr),
    m_nextClipboardWindow(nullptr),
    m_ownClipboard(false),
    m_desks(nullptr),
    m_keyState(nullptr),
    m_hasMouse(GetSystemMetrics(SM_MOUSEPRESENT) != 0),
    m_showingMouse(false),
    m_events(events),
    m_dropWindow(nullptr),
    m_dropWindowSize(20)
{
    assert(s_windowInstance != nullptr);
    assert(s_screen == nullptr);

    s_screen = this;
    try {
        m_screensaver = new MSWindowsScreenSaver();
        m_desks       = new MSWindowsDesks(
                            m_isPrimary,
                            m_noHooks,
                            m_screensaver,
                            m_events,
                                           [this]() { updateKeysCB(); },
                            stopOnDeskSwitch);
        m_keyState    = new MSWindowsKeyState(m_desks, get_event_target(), m_events);

        updateScreenShape();
        m_class       = createWindowClass();
        m_window      = createWindow(m_class, "InputLeap");
        forceShowCursor();
        LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
        LOG((CLOG_DEBUG "window is 0x%08x", m_window));

        OleInitialize(0);
        m_dropWindow = createDropWindow(m_class, "DropWindow");
        m_dropTarget = new MSWindowsDropTarget();
        RegisterDragDrop(m_dropWindow, m_dropTarget);
    }
    catch (...) {
        delete m_keyState;
        delete m_desks;
        delete m_screensaver;
        destroyWindow(m_window);
        destroyClass(m_class);
        s_screen = nullptr;
        throw;
    }

    // install event handlers
    m_events->add_handler(EventType::SYSTEM, m_events->getSystemTarget(),
                          [this](const auto& e){ handle_system_event(e); });

    // install the platform event queue
    m_events->adoptBuffer(new MSWindowsEventQueueBuffer(m_events));
}

MSWindowsScreen::~MSWindowsScreen()
{
    assert(s_screen != nullptr);

    disable();
    m_events->adoptBuffer(nullptr);
    m_events->removeHandler(EventType::SYSTEM, m_events->getSystemTarget());
    delete m_keyState;
    delete m_desks;
    delete m_screensaver;
    destroyWindow(m_window);
    destroyClass(m_class);

    RevokeDragDrop(m_dropWindow);
    m_dropTarget->Release();
    OleUninitialize();
    destroyWindow(m_dropWindow);

    s_screen = nullptr;
}

void
MSWindowsScreen::init(HINSTANCE windowInstance)
{
    assert(s_windowInstance == nullptr);
    assert(windowInstance != nullptr);

    s_windowInstance = windowInstance;
}

HINSTANCE
MSWindowsScreen::getWindowInstance()
{
    return s_windowInstance;
}

void
MSWindowsScreen::enable()
{
    assert(m_isOnScreen == m_isPrimary);

    // we need to poll some things to fix them
    m_fixTimer = m_events->newTimer(1.0, nullptr);
    m_events->add_handler(EventType::TIMER, m_fixTimer,
                          [this](const auto& e){ handle_fixes(); });

    // install our clipboard snooper
    m_nextClipboardWindow = SetClipboardViewer(m_window);

    // track the active desk and (re)install the hooks
    m_desks->enable();

    if (m_isPrimary) {
        // set jump zones
        m_hook.setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());

        // watch jump zones
        m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);
    }
    else {
        // prevent the system from entering power saving modes.  if
        // it did we'd be forced to disconnect from the server and
        // the server would not be able to wake us up.
        ArchMiscWindows::addBusyState(ArchMiscWindows::kSYSTEM);
    }
}

void
MSWindowsScreen::disable()
{
    // stop tracking the active desk
    m_desks->disable();

    if (m_isPrimary) {
        // disable hooks
        m_hook.setMode(kHOOK_DISABLE);

        // enable special key sequences on win95 family
        enableSpecialKeys(true);
    }
    else {
        // allow the system to enter power saving mode
        ArchMiscWindows::removeBusyState(ArchMiscWindows::kSYSTEM |
                            ArchMiscWindows::kDISPLAY);
    }

    // tell key state
    m_keyState->disable();

    // stop snooping the clipboard
    ChangeClipboardChain(m_window, m_nextClipboardWindow);
    m_nextClipboardWindow = nullptr;

    // uninstall fix timer
    if (m_fixTimer != nullptr) {
        m_events->removeHandler(EventType::TIMER, m_fixTimer);
        m_events->deleteTimer(m_fixTimer);
        m_fixTimer = nullptr;
    }

    m_isOnScreen = m_isPrimary;
    forceShowCursor();
}

void
MSWindowsScreen::enter()
{
    m_desks->enter();
    if (m_isPrimary) {
        // enable special key sequences on win95 family
        enableSpecialKeys(true);

        // watch jump zones
        m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);

        // all messages prior to now are invalid
        nextMark();

        m_primaryKeyDownList.clear();
    }
    else {
        // Entering a secondary screen. Ensure that no screensaver is active
        // and that the screen is not in powersave mode.
        ArchMiscWindows::wakeupDisplay();

        if (m_screensaver != nullptr && m_screensaverActive)
        {
            m_screensaver->deactivate();
            m_screensaverActive = 0;
        }
    }

    // now on screen
    m_isOnScreen = true;
    forceShowCursor();
}

bool
MSWindowsScreen::leave()
{
    // get keyboard layout of foreground window.  we'll use this
    // keyboard layout for translating keys sent to clients.
    HWND window  = GetForegroundWindow();
    DWORD thread = GetWindowThreadProcessId(window, nullptr);
    m_keyLayout  = GetKeyboardLayout(thread);

    // tell the key mapper about the keyboard layout
    m_keyState->setKeyLayout(m_keyLayout);

    // tell desk that we're leaving and tell it the keyboard layout
    m_desks->leave(m_keyLayout);

    if (m_isPrimary) {

        // warp to center
        LOG((CLOG_DEBUG1 "warping cursor to center: %+d, %+d", m_xCenter, m_yCenter));
        warpCursor(m_xCenter, m_yCenter);

        // disable special key sequences on win95 family
        enableSpecialKeys(false);

        // all messages prior to now are invalid
        nextMark();

        // remember the modifier state.  this is the modifier state
        // reflected in the internal keyboard state.
        m_keyState->saveModifiers();

        m_hook.setMode(kHOOK_RELAY_EVENTS);

        m_primaryKeyDownList.clear();
        for (KeyButton i = 0; i < IKeyState::kNumButtons; ++i) {
            if (m_keyState->isKeyDown(i)) {
                m_primaryKeyDownList.push_back(i);
                LOG((CLOG_DEBUG1 "key button %d is down before leaving to another screen", i));
            }
        }
    }

    // now off screen
    m_isOnScreen = false;
    forceShowCursor();

    if (isDraggingStarted() && !m_isPrimary) {
        m_sendDragThread = new Thread([this](){ send_drag_thread(); });
    }

    return true;
}

void MSWindowsScreen::send_drag_thread()
{
    std::string& draggingFilename = getDraggingFilename();
    size_t size = draggingFilename.size();

    if (draggingFilename.empty() == false) {
        ClientApp& app = ClientApp::instance();
        Client* client = app.getClientPtr();
        std::uint32_t fileCount = 1;
        LOG((CLOG_DEBUG "send dragging info to server: %s", draggingFilename.c_str()));
        client->sendDragInfo(fileCount, draggingFilename, size);
        LOG((CLOG_DEBUG "send dragging file to server"));
        client->sendFileToServer(draggingFilename.c_str());
    }

    m_draggingStarted = false;
}

bool
MSWindowsScreen::setClipboard(ClipboardID, const IClipboard* src)
{
    MSWindowsClipboard dst(m_window);
    if (src != nullptr) {
        // save clipboard data
        return Clipboard::copy(&dst, src);
    }
    else {
        // assert clipboard ownership
        if (!dst.open(0)) {
            return false;
        }
        dst.empty();
        dst.close();
        return true;
    }
}

void
MSWindowsScreen::checkClipboards()
{
    // if we think we own the clipboard but we don't then somebody
    // grabbed the clipboard on this screen without us knowing.
    // tell the server that this screen grabbed the clipboard.
    //
    // this works around bugs in the clipboard viewer chain.
    // sometimes NT will simply never send WM_DRAWCLIPBOARD
    // messages for no apparent reason and rebooting fixes the
    // problem.  since we don't want a broken clipboard until the
    // next reboot we do this double check.  clipboard ownership
    // won't be reflected on other screens until we leave but at
    // least the clipboard itself will work.
    if (m_ownClipboard && !MSWindowsClipboard::is_owned_by_us()) {
        LOG((CLOG_DEBUG "clipboard changed: lost ownership and no notification received"));
        m_ownClipboard = false;
        sendClipboardEvent(EventType::CLIPBOARD_GRABBED, kClipboardClipboard);
        sendClipboardEvent(EventType::CLIPBOARD_GRABBED, kClipboardSelection);
    }
}

void
MSWindowsScreen::openScreensaver(bool notify)
{
    assert(m_screensaver != nullptr);

    m_screensaverNotify = notify;
    if (m_screensaverNotify) {
        m_desks->installScreensaverHooks(true);
    }
    else if (m_screensaver) {
        m_screensaver->disable();
    }
}

void
MSWindowsScreen::closeScreensaver()
{
    if (m_screensaver != nullptr) {
        if (m_screensaverNotify) {
            m_desks->installScreensaverHooks(false);
        }
        else {
            m_screensaver->enable();
        }
    }
    m_screensaverNotify = false;
}

void
MSWindowsScreen::screensaver(bool activate)
{
    assert(m_screensaver != nullptr);
    if (m_screensaver==nullptr) return;

    if (activate) {
        m_screensaver->activate();
    }
    else {
        m_screensaver->deactivate();
    }
}

void
MSWindowsScreen::resetOptions()
{
    m_desks->resetOptions();
}

void
MSWindowsScreen::setOptions(const OptionsList& options)
{
    m_desks->setOptions(options);
}

void MSWindowsScreen::setSequenceNumber(std::uint32_t seqNum)
{
    m_sequenceNumber = seqNum;
}

bool
MSWindowsScreen::isPrimary() const
{
    return m_isPrimary;
}

const void* MSWindowsScreen::get_event_target() const
{
    return this;
}

bool
MSWindowsScreen::getClipboard(ClipboardID, IClipboard* dst) const
{
    MSWindowsClipboard src(m_window);
    Clipboard::copy(dst, &src);
    return true;
}

void MSWindowsScreen::getShape(std::int32_t& x, std::int32_t& y, std::int32_t& w,
                               std::int32_t& h) const
{
    assert(m_class != 0);

    x = m_x;
    y = m_y;
    w = m_w;
    h = m_h;
}

void MSWindowsScreen::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    m_desks->getCursorPos(x, y);
}

void MSWindowsScreen::reconfigure(std::uint32_t activeSides)
{
    assert(m_isPrimary);

    LOG((CLOG_DEBUG "active sides: %x", activeSides));
    m_hook.setSides(activeSides);
}

void MSWindowsScreen::warpCursor(std::int32_t x, std::int32_t y)
{
    // warp mouse
    warpCursorNoFlush(x, y);

    // remove all input events before and including warp
    MSG msg;
    while (PeekMessage(&msg, nullptr, INPUTLEAP_MSG_INPUT_FIRST,
                                INPUTLEAP_MSG_INPUT_LAST, PM_REMOVE)) {
        // do nothing
    }

    // save position to compute delta of next motion
    saveMousePosition(x, y);
}

void MSWindowsScreen::saveMousePosition(std::int32_t x, std::int32_t y) {
    m_xCursor = x;
    m_yCursor = y;

    LOG((CLOG_DEBUG5 "saved mouse position for next delta: %+d,%+d", x,y));
}

std::uint32_t MSWindowsScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
    // only allow certain modifiers
    if ((mask & ~(KeyModifierShift | KeyModifierControl |
                  KeyModifierAlt   | KeyModifierSuper)) != 0) {
        // this should be a warning, but this can confuse users,
        // as this warning happens almost always.
        LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
        return 0;
    }

    // fail if no keys
    if (key == kKeyNone && mask == 0) {
        return 0;
    }

    // convert to win32
    UINT modifiers = 0;
    if ((mask & KeyModifierShift) != 0) {
        modifiers |= MOD_SHIFT;
    }
    if ((mask & KeyModifierControl) != 0) {
        modifiers |= MOD_CONTROL;
    }
    if ((mask & KeyModifierAlt) != 0) {
        modifiers |= MOD_ALT;
    }
    if ((mask & KeyModifierSuper) != 0) {
        modifiers |= MOD_WIN;
    }
    UINT vk = m_keyState->mapKeyToVirtualKey(key);
    if (key != kKeyNone && vk == 0) {
        // can't map key
        // this should be a warning, but this can confuse users,
        // as this warning happens almost always.
        LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
        return 0;
    }

    // choose hotkey id
    std::uint32_t id;
    if (!m_oldHotKeyIDs.empty()) {
        id = m_oldHotKeyIDs.back();
        m_oldHotKeyIDs.pop_back();
    }
    else {
        //id = m_hotKeys.size() + 1;
        id = (std::uint32_t)m_hotKeys.size() + 1;
    }

    // if this hot key has modifiers only then we'll handle it specially
    bool err;
    if (key == kKeyNone) {
        // check if already registered
        err = (m_hotKeyToIDMap.count(HotKeyItem(vk, modifiers)) > 0);
    }
    else {
        // register with OS
        err = (RegisterHotKey(nullptr, id, modifiers, vk) == 0);
    }

    if (!err) {
        m_hotKeys.insert(std::make_pair(id, HotKeyItem(vk, modifiers)));
        m_hotKeyToIDMap[HotKeyItem(vk, modifiers)] = id;
    }
    else {
        m_oldHotKeyIDs.push_back(id);
        m_hotKeys.erase(id);
        LOG((CLOG_WARN "failed to register hotkey %s (id=%04x mask=%04x)", inputleap::KeyMap::formatKey(key, mask).c_str(), key, mask));
        return 0;
    }

    LOG((CLOG_DEBUG "registered hotkey %s (id=%04x mask=%04x) as id=%d", inputleap::KeyMap::formatKey(key, mask).c_str(), key, mask, id));
    return id;
}

void MSWindowsScreen::unregisterHotKey(std::uint32_t id)
{
    // look up hotkey
    HotKeyMap::iterator i = m_hotKeys.find(id);
    if (i == m_hotKeys.end()) {
        return;
    }

    // unregister with OS
    bool err;
    if (i->second.getVirtualKey() != 0) {
        err = !UnregisterHotKey(nullptr, id);
    }
    else {
        err = false;
    }
    if (err) {
        LOG((CLOG_WARN "failed to unregister hotkey id=%d", id));
    }
    else {
        LOG((CLOG_DEBUG "unregistered hotkey id=%d", id));
    }

    // discard hot key from map and record old id for reuse
    m_hotKeyToIDMap.erase(i->second);
    m_hotKeys.erase(i);
    m_oldHotKeyIDs.push_back(id);
}

void
MSWindowsScreen::fakeInputBegin()
{
    assert(m_isPrimary);

    if (!m_isOnScreen) {
        m_keyState->useSavedModifiers(true);
    }
    m_desks->fakeInputBegin();
}

void
MSWindowsScreen::fakeInputEnd()
{
    assert(m_isPrimary);

    m_desks->fakeInputEnd();
    if (!m_isOnScreen) {
        m_keyState->useSavedModifiers(false);
    }
}

std::int32_t MSWindowsScreen::getJumpZoneSize() const
{
    return 1;
}

bool MSWindowsScreen::isAnyMouseButtonDown(std::uint32_t& buttonID) const
{
    static const char* buttonToName[] = {
        "<invalid>",
        "Left Button",
        "Middle Button",
        "Right Button",
        "X Button 1",
        "X Button 2"
    };

    for (std::uint32_t i = 1; i < sizeof(m_buttons) / sizeof(m_buttons[0]); ++i) {
        if (m_buttons[i]) {
            buttonID = i;
            LOG((CLOG_DEBUG "locked by \"%s\"", buttonToName[i]));
            return true;
        }
    }

    return false;
}

void MSWindowsScreen::getCursorCenter(std::int32_t& x, std::int32_t& y) const
{
    x = m_xCenter;
    y = m_yCenter;
}

void
MSWindowsScreen::fakeMouseButton(ButtonID id, bool press)
{
    m_desks->fakeMouseButton(id, press);

    if (id == kButtonLeft) {
        if (press) {
            m_buttons[kButtonLeft] = true;
        }
        else {
            m_buttons[kButtonLeft] = false;
            m_fakeDraggingStarted = false;
            m_draggingStarted = false;
        }
    }
}

void MSWindowsScreen::fakeMouseMove(std::int32_t x, std::int32_t y)
{
    m_desks->fakeMouseMove(x, y);
    if (m_buttons[kButtonLeft]) {
        m_draggingStarted = true;
    }
}

void MSWindowsScreen::fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
    m_desks->fakeMouseRelativeMove(dx, dy);
}

void MSWindowsScreen::fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const
{
    m_desks->fakeMouseWheel(xDelta, yDelta);
}

void
MSWindowsScreen::updateKeys()
{
    m_desks->updateKeys();
}

void
MSWindowsScreen::fakeKeyDown(KeyID id, KeyModifierMask mask,
                KeyButton button)
{
    PlatformScreen::fakeKeyDown(id, mask, button);
    updateForceShowCursor();
}

bool MSWindowsScreen::fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                                    KeyButton button)
{
    bool result = PlatformScreen::fakeKeyRepeat(id, mask, count, button);
    updateForceShowCursor();
    return result;
}

bool
MSWindowsScreen::fakeKeyUp(KeyButton button)
{
    bool result = PlatformScreen::fakeKeyUp(button);
    updateForceShowCursor();
    return result;
}

void
MSWindowsScreen::fakeAllKeysUp()
{
    PlatformScreen::fakeAllKeysUp();
    updateForceShowCursor();
}

HCURSOR
MSWindowsScreen::createBlankCursor() const
{
    // create a transparent cursor
    int cw = GetSystemMetrics(SM_CXCURSOR);
    int ch = GetSystemMetrics(SM_CYCURSOR);

    std::uint8_t* cursorAND = new std::uint8_t[ch * ((cw + 31) >> 2)];
    std::uint8_t* cursorXOR = new std::uint8_t[ch * ((cw + 31) >> 2)];
    memset(cursorAND, 0xff, ch * ((cw + 31) >> 2));
    memset(cursorXOR, 0x00, ch * ((cw + 31) >> 2));
    HCURSOR c = CreateCursor(s_windowInstance, 0, 0, cw, ch, cursorAND, cursorXOR);
    delete[] cursorXOR;
    delete[] cursorAND;
    return c;
}

void
MSWindowsScreen::destroyCursor(HCURSOR cursor) const
{
    if (cursor != nullptr) {
        DestroyCursor(cursor);
    }
}

ATOM
MSWindowsScreen::createWindowClass() const
{
    WNDCLASSEX classInfo;
    classInfo.cbSize        = sizeof(classInfo);
    classInfo.style         = CS_DBLCLKS | CS_NOCLOSE;
    classInfo.lpfnWndProc   = &MSWindowsScreen::wndProc;
    classInfo.cbClsExtra    = 0;
    classInfo.cbWndExtra    = 0;
    classInfo.hInstance     = s_windowInstance;
    classInfo.hIcon = nullptr;
    classInfo.hCursor = nullptr;
    classInfo.hbrBackground = nullptr;
    classInfo.lpszMenuName = nullptr;
    classInfo.lpszClassName = "InputLeap";
    classInfo.hIconSm = nullptr;
    return RegisterClassEx(&classInfo);
}

void
MSWindowsScreen::destroyClass(ATOM windowClass) const
{
    if (windowClass != 0) {
        UnregisterClass(MAKEINTATOM(windowClass), s_windowInstance);
    }
}

HWND
MSWindowsScreen::createWindow(ATOM windowClass, const char* name) const
{
    HWND window = CreateWindowEx(WS_EX_TOPMOST |
                                    WS_EX_TRANSPARENT |
                                    WS_EX_TOOLWINDOW,
                                MAKEINTATOM(windowClass),
                                name,
                                WS_POPUP,
                                0, 0, 1, 1,
                                nullptr, nullptr,
                                s_windowInstance,
                                nullptr);
    if (window == nullptr) {
        LOG((CLOG_ERR "failed to create window: %d", GetLastError()));
        throw XScreenOpenFailure();
    }
    return window;
}

HWND
MSWindowsScreen::createDropWindow(ATOM windowClass, const char* name) const
{
    HWND window = CreateWindowEx(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_ACCEPTFILES,
                                 MAKEINTATOM(m_class),
                                 name,
                                 WS_POPUP,
                                 0, 0, m_dropWindowSize, m_dropWindowSize,
                                 nullptr, nullptr,
                                 s_windowInstance,
                                 nullptr);

    if (window == nullptr) {
        LOG((CLOG_ERR "failed to create drop window: %d", GetLastError()));
        throw XScreenOpenFailure();
    }

    return window;
}

void
MSWindowsScreen::destroyWindow(HWND hwnd) const
{
    if (hwnd != nullptr) {
        DestroyWindow(hwnd);
    }
}

void MSWindowsScreen::sendEvent(EventType type, EventDataBase* data)
{
    m_events->add_event(type, get_event_target(), data);
}

void MSWindowsScreen::sendClipboardEvent(EventType type, ClipboardID id)
{
    ClipboardInfo info;
    info.m_id = id;
    info.m_sequenceNumber = m_sequenceNumber;
    sendEvent(type, create_event_data<ClipboardInfo>(info));
}

void MSWindowsScreen::handle_system_event(const Event& event)
{
    MSG* msg = event.get_data_as<MSG*>();
    assert(msg != nullptr);

    if (ArchMiscWindows::processDialog(msg)) {
        return;
    }
    if (onPreDispatch(msg->hwnd, msg->message, msg->wParam, msg->lParam)) {
        return;
    }
    TranslateMessage(msg);
    DispatchMessage(msg);
}

void
MSWindowsScreen::updateButtons()
{
    int numButtons               = GetSystemMetrics(SM_CMOUSEBUTTONS);
    m_buttons[kButtonNone]       = false;
    m_buttons[kButtonLeft]       = (GetKeyState(VK_LBUTTON)  < 0);
    m_buttons[kButtonRight]      = (GetKeyState(VK_RBUTTON)  < 0);
    m_buttons[kButtonMiddle]     = (GetKeyState(VK_MBUTTON)  < 0);
    m_buttons[kButtonExtra0]     = (numButtons >= 4) &&
                                   (GetKeyState(VK_XBUTTON1) < 0);
    m_buttons[kButtonExtra1]     = (numButtons >= 5) &&
                                   (GetKeyState(VK_XBUTTON2) < 0);
}

IKeyState*
MSWindowsScreen::getKeyState() const
{
    return m_keyState;
}

bool
MSWindowsScreen::onPreDispatch(HWND hwnd,
                UINT message, WPARAM wParam, LPARAM lParam)
{
    // handle event
    switch (message) {
    case INPUTLEAP_MSG_SCREEN_SAVER:
        return onScreensaver(wParam != 0);

    case INPUTLEAP_MSG_DEBUG:
        LOG((CLOG_DEBUG1 "hook: 0x%08x 0x%08x", wParam, lParam));
        return true;
    }

    if (m_isPrimary) {
        return onPreDispatchPrimary(hwnd, message, wParam, lParam);
    }

    return false;
}

bool
MSWindowsScreen::onPreDispatchPrimary(HWND,
                UINT message, WPARAM wParam, LPARAM lParam)
{
    LOG((CLOG_DEBUG5 "handling pre-dispatch primary"));

    // handle event
    switch (message) {
    case INPUTLEAP_MSG_MARK:
        return onMark(static_cast<std::uint32_t>(wParam));

    case INPUTLEAP_MSG_KEY:
        return onKey(wParam, lParam);

    case INPUTLEAP_MSG_MOUSE_BUTTON:
        return onMouseButton(wParam, lParam);

    case INPUTLEAP_MSG_MOUSE_MOVE:
        return onMouseMove(static_cast<std::int32_t>(wParam), static_cast<std::int32_t>(lParam));

    case INPUTLEAP_MSG_MOUSE_WHEEL:
        return onMouseWheel(static_cast<std::int32_t>(lParam), static_cast<std::int32_t>(wParam));

    case INPUTLEAP_MSG_PRE_WARP:
        {
            // save position to compute delta of next motion
            saveMousePosition(static_cast<std::int32_t>(wParam), static_cast<std::int32_t>(lParam));

            // we warped the mouse.  discard events until we find the
            // matching post warp event.  see warpCursorNoFlush() for
            // where the events are sent.  we discard the matching
            // post warp event and can be sure we've skipped the warp
            // event.
            MSG msg;
            do {
                GetMessage(&msg, nullptr, INPUTLEAP_MSG_MOUSE_MOVE,
                                        INPUTLEAP_MSG_POST_WARP);
            } while (msg.message != INPUTLEAP_MSG_POST_WARP);
        }
        return true;

    case INPUTLEAP_MSG_POST_WARP:
        LOG((CLOG_WARN "unmatched post warp"));
        return true;

    case WM_HOTKEY:
        // we discard these messages.  we'll catch the hot key in the
        // regular key event handling, where we can detect both key
        // press and release.  we only register the hot key so no other
        // app will act on the key combination.
        break;
    }

    return false;
}

bool
MSWindowsScreen::onEvent(HWND, UINT msg,
                WPARAM wParam, LPARAM lParam, LRESULT* result)
{
    switch (msg) {
    case WM_DRAWCLIPBOARD:
        // first pass on the message
        if (m_nextClipboardWindow != nullptr) {
            SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
        }

        // now handle the message
        return onClipboardChange();

    case WM_CHANGECBCHAIN:
        if (m_nextClipboardWindow == (HWND)wParam) {
            m_nextClipboardWindow = (HWND)lParam;
            LOG((CLOG_DEBUG "clipboard chain: new next: 0x%08x", m_nextClipboardWindow));
        }
        else if (m_nextClipboardWindow != nullptr) {
            SendMessage(m_nextClipboardWindow, msg, wParam, lParam);
        }
        return true;

    case WM_DISPLAYCHANGE:
        return onDisplayChange();

    case WM_POWERBROADCAST:
        switch (wParam) {
        case PBT_APMRESUMEAUTOMATIC:
        case PBT_APMRESUMECRITICAL:
        case PBT_APMRESUMESUSPEND:
            m_events->add_event(EventType::SCREEN_RESUME, get_event_target(), nullptr,
                                Event::kDeliverImmediately);
            break;

        case PBT_APMSUSPEND:
            m_events->add_event(EventType::SCREEN_SUSPEND, get_event_target(), nullptr,
                                Event::kDeliverImmediately);
            break;
        }
        *result = TRUE;
        return true;

    case WM_DEVICECHANGE:
        forceShowCursor();
        break;

    case WM_SETTINGCHANGE:
        if (wParam == SPI_SETMOUSEKEYS) {
            forceShowCursor();
        }
        break;
    }

    return false;
}

bool MSWindowsScreen::onMark(std::uint32_t mark)
{
    m_markReceived = mark;
    return true;
}

bool
MSWindowsScreen::onKey(WPARAM wParam, LPARAM lParam)
{
    static const KeyModifierMask s_ctrlAlt =
        KeyModifierControl | KeyModifierAlt;

    LOG((CLOG_DEBUG1 "event: Key char=%d, vk=0x%02x, nagr=%d, lParam=0x%08x", wParam & 0xffffu, (wParam >> 16) & 0xffu, (wParam & 0x1000000u) ? 1 : 0, lParam));

    // get event info
    KeyButton button         = (KeyButton)((lParam & 0x01ff0000) >> 16);
    bool down                = ((lParam & 0x80000000u) == 0x00000000u);
    bool wasDown             = isKeyDown(button);
    KeyModifierMask oldState = pollActiveModifiers();

    // check for autorepeat
    if (m_keyState->testAutoRepeat(down, (lParam & 0x40000000u), button)) {
        lParam |= 0x40000000u;
    }

    // if the button is zero then guess what the button should be.
    // these are badly synthesized key events and logitech software
    // that maps mouse buttons to keys is known to do this.
    // alternatively, we could just throw these events out.
    if (button == 0) {
        button = m_keyState->virtualKeyToButton((wParam >> 16) & 0xffu);
        if (button == 0) {
            return true;
        }
        wasDown = isKeyDown(button);
    }

    // record keyboard state
    m_keyState->onKey(button, down, oldState);

    if (!down && m_isPrimary && !m_isOnScreen) {
        PrimaryKeyDownList::iterator find = std::find(m_primaryKeyDownList.begin(), m_primaryKeyDownList.end(), button);
        if (find != m_primaryKeyDownList.end()) {
            LOG((CLOG_DEBUG1 "release key button %d on primary", *find));
            m_hook.setMode(kHOOK_WATCH_JUMP_ZONE);
            fakeLocalKey(*find, false);
            m_primaryKeyDownList.erase(find);
            m_hook.setMode(kHOOK_RELAY_EVENTS);
            return true;
        }
    }

    // windows doesn't tell us the modifier key state on mouse or key
    // events so we have to figure it out.  most apps would use
    // GetKeyState() or even GetAsyncKeyState() for that but we can't
    // because our hook doesn't pass on key events for several modifiers.
    // it can't otherwise the system would interpret them normally on
    // the primary screen even when on a secondary screen.  so tapping
    // alt would activate menus and tapping the windows key would open
    // the start menu.  if you don't pass those events on in the hook
    // then GetKeyState() understandably doesn't reflect the effect of
    // the event.  curiously, neither does GetAsyncKeyState(), which is
    // surprising.
    //
    // so anyway, we have to track the modifier state ourselves for
    // at least those modifiers we don't pass on.  pollActiveModifiers()
    // does that but we have to update the keyboard state before calling
    // pollActiveModifiers() to get the right answer.  but the only way
    // to set the modifier state or to set the up/down state of a key
    // is via onKey().  so we have to call onKey() twice.
    KeyModifierMask state = pollActiveModifiers();
    m_keyState->onKey(button, down, state);

    // check for hot keys
    if (oldState != state) {
        // modifier key was pressed/released
        if (onHotKey(0, lParam)) {
            return true;
        }
    }
    else {
        // non-modifier was pressed/released
        if (onHotKey(wParam, lParam)) {
            return true;
        }
    }

    // stop sending modifier keys over and over again
    if (isModifierRepeat(oldState, state, wParam)) {
        return true;
    }

    // ignore message if posted prior to last mark change
    if (!ignore()) {
        // check for ctrl+alt+del.  we do not want to pass that to the
        // client.  the user can use ctrl+alt+pause to emulate it.
        UINT virtKey = (wParam >> 16) & 0xffu;
        if (virtKey == VK_DELETE && (state & s_ctrlAlt) == s_ctrlAlt) {
            LOG((CLOG_DEBUG "discard ctrl+alt+del"));
            return true;
        }

        // check for ctrl+alt+del emulation
        if ((virtKey == VK_PAUSE || virtKey == VK_CANCEL) &&
            (state & s_ctrlAlt) == s_ctrlAlt) {
            LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
            // switch wParam and lParam to be as if VK_DELETE was
            // pressed or released.  when mapping the key we require that
            // we not use AltGr (the 0x10000 flag in wParam) and we not
            // use the keypad delete key (the 0x01000000 flag in lParam).
            wParam  = (VK_DELETE << 16) | 0x01000000u;
            lParam &= 0xfe000000;
            lParam |= m_keyState->virtualKeyToButton(VK_DELETE) << 16;
            lParam |= 0x01000001;
        }

        // process key
        KeyModifierMask mask;
        KeyID key = m_keyState->mapKeyFromEvent(wParam, lParam, &mask);
        button    = static_cast<KeyButton>((lParam & 0x01ff0000u) >> 16);
        if (key != kKeyNone) {
            // do it
            m_keyState->sendKeyEvent(get_event_target(),
                            ((lParam & 0x80000000u) == 0),
                            ((lParam & 0x40000000u) != 0),
                            key, mask, (std::int32_t)(lParam & 0xffff), button);
        }
        else {
            LOG((CLOG_DEBUG1 "cannot map key"));
        }
    }

    return true;
}

bool
MSWindowsScreen::onHotKey(WPARAM wParam, LPARAM lParam)
{
    // get the key info
    KeyModifierMask state = getActiveModifiers();
    UINT virtKey   = (wParam >> 16) & 0xffu;
    UINT modifiers = 0;
    if ((state & KeyModifierShift) != 0) {
        modifiers |= MOD_SHIFT;
    }
    if ((state & KeyModifierControl) != 0) {
        modifiers |= MOD_CONTROL;
    }
    if ((state & KeyModifierAlt) != 0) {
        modifiers |= MOD_ALT;
    }
    if ((state & KeyModifierSuper) != 0) {
        modifiers |= MOD_WIN;
    }

    // find the hot key id
    HotKeyToIDMap::const_iterator i =
        m_hotKeyToIDMap.find(HotKeyItem(virtKey, modifiers));
    if (i == m_hotKeyToIDMap.end()) {
        return false;
    }

    // find what kind of event
    EventType type;
    if ((lParam & 0x80000000u) == 0u) {
        if ((lParam & 0x40000000u) != 0u) {
            // ignore key repeats but it counts as a hot key
            return true;
        }
        type = EventType::PRIMARY_SCREEN_HOTKEY_DOWN;
    }
    else {
        type = EventType::PRIMARY_SCREEN_HOTKEY_UP;
    }

    // generate event
    m_events->add_event(type, get_event_target(),
                        create_event_data<HotKeyInfo>(HotKeyInfo{i->second}));

    return true;
}

bool
MSWindowsScreen::onMouseButton(WPARAM wParam, LPARAM lParam)
{
    // get which button
    bool pressed    = mapPressFromEvent(wParam, lParam);
    ButtonID button = mapButtonFromEvent(wParam, lParam);

    // keep our shadow key state up to date
    if (button >= kButtonLeft && button <= kButtonExtra0 + 1) {
        if (pressed) {
            m_buttons[button] = true;
            if (button == kButtonLeft) {
                m_draggingFilename.clear();
                LOG((CLOG_DEBUG2 "dragging filename is cleared"));
            }
        }
        else {
            m_buttons[button] = false;
            if (m_draggingStarted && button == kButtonLeft) {
                m_draggingStarted = false;
            }
        }
    }

    // ignore message if posted prior to last mark change
    if (!ignore()) {
        KeyModifierMask mask = m_keyState->getActiveModifiers();
        if (pressed) {
            LOG((CLOG_DEBUG1 "event: button press button=%d", button));
            if (button != kButtonNone) {
                sendEvent(EventType::PRIMARY_SCREEN_BUTTON_DOWN,
                          create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
            }
        }
        else {
            LOG((CLOG_DEBUG1 "event: button release button=%d", button));
            if (button != kButtonNone) {
                sendEvent(EventType::PRIMARY_SCREEN_BUTTON_UP,
                          create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
            }
        }
    }

    return true;
}

// here's how mouse movements are sent across the network to a client:
//   1. InputLeap checks the mouse position on server screen
//   2. records the delta (current x,y minus last x,y)
//   3. records the current x,y as "last" (so we can calc delta next time)
//   4. on the server, puts the cursor back to the center of the screen
//      - remember the cursor is hidden on the server at this point
//      - this actually records the current x,y as "last" a second time (it seems)
//   5. sends the delta movement to the client (could be +1,+1 or -1,+4 for example)
bool MSWindowsScreen::onMouseMove(std::int32_t mx, std::int32_t my)
{
    // compute motion delta (relative to the last known
    // mouse position)
    std::int32_t x = mx - m_xCursor;
    std::int32_t y = my - m_yCursor;

    LOG((CLOG_DEBUG3
        "mouse move - motion delta: %+d=(%+d - %+d),%+d=(%+d - %+d)",
        x, mx, m_xCursor, y, my, m_yCursor));

    // ignore if the mouse didn't move or if message posted prior
    // to last mark change.
    if (ignore() || (x == 0 && y == 0)) {
        return true;
    }

    // save position to compute delta of next motion
    saveMousePosition(mx, my);

    if (m_isOnScreen) {

        // motion on primary screen
        sendEvent(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY,
                  create_event_data<MotionInfo>(MotionInfo{m_xCursor, m_yCursor}));

        if (m_buttons[kButtonLeft] == true && m_draggingStarted == false) {
            m_draggingStarted = true;
        }
    }
    else
    {
        // the motion is on the secondary screen, so we warp mouse back to
        // center on the server screen. if we don't do this, then the mouse
        // will always try to return to the original entry point on the
        // secondary screen.
        LOG((CLOG_DEBUG5 "warping server cursor to center: %+d,%+d", m_xCenter, m_yCenter));
        warpCursorNoFlush(m_xCenter, m_yCenter);

        // examine the motion.  if it's about the distance
        // from the center of the screen to an edge then
        // it's probably a bogus motion that we want to
        // ignore (see warpCursorNoFlush() for a further
        // description).
        static std::int32_t bogusZoneSize = 10;
        if (-x + bogusZoneSize > m_xCenter - m_x ||
             x + bogusZoneSize > m_x + m_w - m_xCenter ||
            -y + bogusZoneSize > m_yCenter - m_y ||
             y + bogusZoneSize > m_y + m_h - m_yCenter) {

            LOG((CLOG_DEBUG "dropped bogus delta motion: %+d,%+d", x, y));
        }
        else {
            // send motion
            sendEvent(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY,
                      create_event_data<MotionInfo>(MotionInfo{x, y}));
        }
    }

    return true;
}

bool MSWindowsScreen::onMouseWheel(std::int32_t xDelta, std::int32_t yDelta)
{
    // ignore message if posted prior to last mark change
    if (!ignore()) {
        LOG((CLOG_DEBUG1 "event: button wheel delta=%+d,%+d", xDelta, yDelta));
        sendEvent(EventType::PRIMARY_SCREEN_WHEEL,
                  create_event_data<WheelInfo>(WheelInfo{xDelta, yDelta}));
    }
    return true;
}

bool
MSWindowsScreen::onScreensaver(bool activated)
{
    // ignore this message if there are any other screen saver
    // messages already in the queue.  this is important because
    // our checkStarted() function has a deliberate delay, so it
    // can't respond to events at full CPU speed and will fall
    // behind if a lot of screen saver events are generated.
    // that can easily happen because windows will continually
    // send SC_SCREENSAVE until the screen saver starts, even if
    // the screen saver is disabled!
    MSG msg;
    if (PeekMessage(&msg, nullptr, INPUTLEAP_MSG_SCREEN_SAVER,
                        INPUTLEAP_MSG_SCREEN_SAVER, PM_NOREMOVE)) {
        return true;
    }

    if (activated) {
        if (!m_screensaverActive &&
            m_screensaver->checkStarted(INPUTLEAP_MSG_SCREEN_SAVER, FALSE, 0)) {
            m_screensaverActive = true;
            sendEvent(EventType::PRIMARY_SCREEN_SAVER_ACTIVATED);

            // enable display power down
            ArchMiscWindows::removeBusyState(ArchMiscWindows::kDISPLAY);
        }
    }
    else {
        if (m_screensaverActive) {
            m_screensaverActive = false;
            sendEvent(EventType::PRIMARY_SCREEN_SAVER_DEACTIVATED);

            // disable display power down
            ArchMiscWindows::addBusyState(ArchMiscWindows::kDISPLAY);
        }
    }

    return true;
}

bool
MSWindowsScreen::onDisplayChange()
{
    // screen resolution may have changed.  save old shape.
    std::int32_t xOld = m_x, yOld = m_y, wOld = m_w, hOld = m_h;

    // update shape
    updateScreenShape();

    // do nothing if resolution hasn't changed
    if (xOld != m_x || yOld != m_y || wOld != m_w || hOld != m_h) {
        if (m_isPrimary) {
            // warp mouse to center if off screen
            if (!m_isOnScreen) {

                LOG((CLOG_DEBUG1 "warping cursor to center: %+d, %+d", m_xCenter, m_yCenter));
                warpCursor(m_xCenter, m_yCenter);
            }

            // tell hook about resize if on screen
            else {
                m_hook.setZone(m_x, m_y, m_w, m_h, getJumpZoneSize());
            }
        }

        // send new screen info
        sendEvent(EventType::SCREEN_SHAPE_CHANGED);

        LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_multimon ? "(multi-monitor)" : ""));
    }

    return true;
}

bool
MSWindowsScreen::onClipboardChange()
{
    // now notify client that somebody changed the clipboard (unless
    // we're the owner).
    if (!MSWindowsClipboard::is_owned_by_us()) {
        if (m_ownClipboard) {
            LOG((CLOG_DEBUG "clipboard changed: lost ownership"));
            m_ownClipboard = false;
            sendClipboardEvent(EventType::CLIPBOARD_GRABBED, kClipboardClipboard);
            sendClipboardEvent(EventType::CLIPBOARD_GRABBED, kClipboardSelection);
        }
    }
    else if (!m_ownClipboard) {
        LOG((CLOG_DEBUG "clipboard changed: got ownership"));
        m_ownClipboard = true;
    }

    return true;
}

void MSWindowsScreen::warpCursorNoFlush(std::int32_t x, std::int32_t y)
{
    // send an event that we can recognize before the mouse warp
    PostThreadMessage(GetCurrentThreadId(), INPUTLEAP_MSG_PRE_WARP, x, y);

    // warp mouse.  hopefully this inserts a mouse motion event
    // between the previous message and the following message.
    SetCursorPos(x, y);

    // check to see if the mouse pos was set correctly
    POINT cursorPos;
    GetCursorPos(&cursorPos);

    // there is a bug or round error in SetCursorPos and GetCursorPos on
    // a high DPI setting. The check here is for Vista/7 login screen.
    // since this feature is mainly for client, so only check on client.
    if (!isPrimary()) {
        if ((cursorPos.x != x) && (cursorPos.y != y)) {
            LOG((CLOG_DEBUG "SetCursorPos did not work; using fakeMouseMove instead"));
            LOG((CLOG_DEBUG "cursor pos %d, %d expected pos %d, %d", cursorPos.x, cursorPos.y, x, y));
            // when at Vista/7 login screen, SetCursorPos does not work (which could be
            // an MS security feature). instead we can use fakeMouseMove, which calls
            // mouse_event.
            // IMPORTANT: as of implementing this function, it has an annoying side
            // effect; instead of the mouse returning to the correct exit point, it
            // returns to the center of the screen. this could have something to do with
            // the center screen warping technique used (see comments for onMouseMove
            // definition).
            fakeMouseMove(x, y);
        }
    }

    // yield the CPU.  there's a race condition when warping:
    //   a hardware mouse event occurs
    //   the mouse hook is not called because that process doesn't have the CPU
    //   we send PRE_WARP, SetCursorPos(), send POST_WARP
    //   we process all of those events and update m_x, m_y
    //   we finish our time slice
    //   the hook is called
    //   the hook sends us a mouse event from the pre-warp position
    //   we get the CPU
    //   we compute a bogus warp
    // we need the hook to process all mouse events that occur
    // before we warp before we do the warp but i'm not sure how
    // to guarantee that.  yielding the CPU here may reduce the
    // chance of undesired behavior.  we'll also check for very
    // large motions that look suspiciously like about half width
    // or height of the screen.
    inputleap::this_thread_sleep(0.0);

    // send an event that we can recognize after the mouse warp
    PostThreadMessage(GetCurrentThreadId(), INPUTLEAP_MSG_POST_WARP, 0, 0);
}

void
MSWindowsScreen::nextMark()
{
    // next mark
    ++m_mark;

    // mark point in message queue where the mark was changed
    PostThreadMessage(GetCurrentThreadId(), INPUTLEAP_MSG_MARK, m_mark, 0);
}

bool
MSWindowsScreen::ignore() const
{
    return (m_mark != m_markReceived);
}

void
MSWindowsScreen::updateScreenShape()
{
    // get shape and center
    m_w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    m_h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    m_x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    m_y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    m_xCenter = GetSystemMetrics(SM_CXSCREEN) >> 1;
    m_yCenter = GetSystemMetrics(SM_CYSCREEN) >> 1;

    // check for multiple monitors
    m_multimon = (m_w != GetSystemMetrics(SM_CXSCREEN) ||
                  m_h != GetSystemMetrics(SM_CYSCREEN));

    // tell the desks
    m_desks->setShape(m_x, m_y, m_w, m_h, m_xCenter, m_yCenter, m_multimon);
}

void MSWindowsScreen::handle_fixes()
{
    // fix clipboard chain
    fixClipboardViewer();

    // update keys if keyboard layouts have changed
    if (m_keyState->didGroupsChange()) {
        updateKeys();
    }
}

void
MSWindowsScreen::fixClipboardViewer()
{
    // XXX -- disable this code for now.  somehow it can cause an infinite
    // recursion in the WM_DRAWCLIPBOARD handler.  either we're sending
    // the message to our own window or some window farther down the chain
    // forwards the message to our window or a window farther up the chain.
    // i'm not sure how that could happen.  the m_nextClipboardWindow = nullptr
    // was not in the code that infinite loops and may fix the bug but i
    // doubt it.
/*
    ChangeClipboardChain(m_window, m_nextClipboardWindow);
    m_nextClipboardWindow = nullptr;
    m_nextClipboardWindow = SetClipboardViewer(m_window);
*/
}

void
MSWindowsScreen::enableSpecialKeys(bool enable) const
{
}

ButtonID
MSWindowsScreen::mapButtonFromEvent(WPARAM msg, LPARAM button) const
{
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCLBUTTONUP:
        return kButtonLeft;

    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_MBUTTONUP:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCMBUTTONUP:
        return kButtonMiddle;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_RBUTTONUP:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCRBUTTONUP:
        return kButtonRight;

    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_XBUTTONUP:
    case WM_NCXBUTTONDOWN:
    case WM_NCXBUTTONDBLCLK:
    case WM_NCXBUTTONUP:
        switch (button) {
        case XBUTTON1:
            if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 4) {
                return kButtonExtra0;
            }
            break;

        case XBUTTON2:
            if (GetSystemMetrics(SM_CMOUSEBUTTONS) >= 5) {
                return kButtonExtra1;
            }
            break;
        }
        return kButtonNone;

    default:
        return kButtonNone;
    }
}

bool
MSWindowsScreen::mapPressFromEvent(WPARAM msg, LPARAM) const
{
    switch (msg) {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:
    case WM_NCMBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
    case WM_NCXBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCMBUTTONDBLCLK:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCXBUTTONDBLCLK:
        return true;

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
    case WM_NCLBUTTONUP:
    case WM_NCMBUTTONUP:
    case WM_NCRBUTTONUP:
    case WM_NCXBUTTONUP:
        return false;

    default:
        return false;
    }
}

void
MSWindowsScreen::updateKeysCB()
{
    // record which keys we think are down
    bool down[IKeyState::kNumButtons];
    bool sendFixes = (isPrimary() && !m_isOnScreen);
    if (sendFixes) {
        for (KeyButton i = 0; i < IKeyState::kNumButtons; ++i) {
            down[i] = m_keyState->isKeyDown(i);
        }
    }

    // update layouts if necessary
    if (m_keyState->didGroupsChange()) {
        PlatformScreen::updateKeyMap();
    }

    // now update the keyboard state
    PlatformScreen::updateKeyState();

    // now see which keys we thought were down but now think are up.
    // send key releases for these keys to the active client.
    if (sendFixes) {
        KeyModifierMask mask = pollActiveModifiers();
        for (KeyButton i = 0; i < IKeyState::kNumButtons; ++i) {
            if (down[i] && !m_keyState->isKeyDown(i)) {
                m_keyState->sendKeyEvent(get_event_target(),
                            false, false, kKeyNone, mask, 1, i);
            }
        }
    }
}

void
MSWindowsScreen::forceShowCursor()
{
    // check for mouse
    m_hasMouse = (GetSystemMetrics(SM_MOUSEPRESENT) != 0);

    // decide if we should show the mouse
    bool showMouse = (!m_hasMouse && !m_isPrimary && m_isOnScreen);

    // show/hide the mouse
    if (showMouse != m_showingMouse) {
        if (showMouse) {
            m_oldMouseKeys.cbSize = sizeof(m_oldMouseKeys);
            m_gotOldMouseKeys =
                (SystemParametersInfo(SPI_GETMOUSEKEYS,
                            m_oldMouseKeys.cbSize,    &m_oldMouseKeys, 0) != 0);
            if (m_gotOldMouseKeys) {
                m_mouseKeys    = m_oldMouseKeys;
                m_showingMouse = true;
                updateForceShowCursor();
            }
        }
        else {
            if (m_gotOldMouseKeys) {
                SystemParametersInfo(SPI_SETMOUSEKEYS,
                            m_oldMouseKeys.cbSize,
                            &m_oldMouseKeys, SPIF_SENDCHANGE);
                m_showingMouse = false;
            }
        }
    }
}

void
MSWindowsScreen::updateForceShowCursor()
{
    DWORD oldFlags = m_mouseKeys.dwFlags;

    // turn on MouseKeys
    m_mouseKeys.dwFlags = MKF_AVAILABLE | MKF_MOUSEKEYSON;

    // make sure MouseKeys is active in whatever state the NumLock is
    // not currently in.
    if ((m_keyState->getActiveModifiers() & KeyModifierNumLock) != 0) {
        m_mouseKeys.dwFlags |= MKF_REPLACENUMBERS;
    }

    // update MouseKeys
    if (oldFlags != m_mouseKeys.dwFlags) {
        SystemParametersInfo(SPI_SETMOUSEKEYS,
                            m_mouseKeys.cbSize, &m_mouseKeys, SPIF_SENDCHANGE);
    }
}

LRESULT CALLBACK
MSWindowsScreen::wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    assert(s_screen != nullptr);

    LRESULT result = 0;
    if (!s_screen->onEvent(hwnd, msg, wParam, lParam, &result)) {
        result = DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return result;
}

void
MSWindowsScreen::fakeLocalKey(KeyButton button, bool press) const
{
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk =  m_keyState->mapButtonToVirtualKey(button);
    DWORD pressFlag = press ? KEYEVENTF_EXTENDEDKEY : KEYEVENTF_KEYUP;
    input.ki.dwFlags = pressFlag;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;
    SendInput(1,&input,sizeof(input));
}

//
// MSWindowsScreen::HotKeyItem
//

MSWindowsScreen::HotKeyItem::HotKeyItem(UINT keycode, UINT mask) :
    m_keycode(keycode),
    m_mask(mask)
{
    // do nothing
}

UINT
MSWindowsScreen::HotKeyItem::getVirtualKey() const
{
    return m_keycode;
}

bool
MSWindowsScreen::HotKeyItem::operator<(const HotKeyItem& x) const
{
    return (m_keycode < x.m_keycode ||
            (m_keycode == x.m_keycode && m_mask < x.m_mask));
}

void
MSWindowsScreen::fakeDraggingFiles(DragFileList fileList)
{
    // possible design flaw: this function stops a "not implemented"
    // exception from being thrown.
}

std::string& MSWindowsScreen::getDraggingFilename()
{
    if (m_draggingStarted) {
        m_dropTarget->clearDraggingFilename();
        m_draggingFilename.clear();

        int halfSize = m_dropWindowSize / 2;

        std::int32_t xPos = m_isPrimary ? m_xCursor : m_xCenter;
        std::int32_t yPos = m_isPrimary ? m_yCursor : m_yCenter;
        xPos = (xPos - halfSize) < 0 ? 0 : xPos - halfSize;
        yPos = (yPos - halfSize) < 0 ? 0 : yPos - halfSize;
        SetWindowPos(
            m_dropWindow,
            HWND_TOPMOST,
            xPos,
            yPos,
            m_dropWindowSize,
            m_dropWindowSize,
            SWP_SHOWWINDOW);

        // TODO: fake these keys properly
        inputleap::this_thread_sleep(.05f); // A tiny sleep here makes the DragEnter event on m_dropWindow trigger much more consistently
        fakeKeyDown(kKeyEscape, 8192, 1);
        fakeKeyUp(1);
        fakeMouseButton(kButtonLeft, false);

        std::string filename;
        DOUBLE timeout = inputleap::current_time_seconds() + .5f;
        while (inputleap::current_time_seconds() < timeout) {
            inputleap::this_thread_sleep(.05f);
            filename = m_dropTarget->getDraggingFilename();
            if (!filename.empty()) {
                break;
            }
        }

        ShowWindow(m_dropWindow, SW_HIDE);

        if (!filename.empty()) {
            if (DragInformation::isFileValid(filename)) {
                m_draggingFilename = filename;
            }
            else {
                LOG((CLOG_ERR "drag file name is invalid: %s", filename.c_str()));
            }
        }

        if (m_draggingFilename.empty()) {
            LOG((CLOG_ERR "failed to get drag file name from OLE"));
        }
    }

    return m_draggingFilename;
}

const std::string&
MSWindowsScreen::getDropTarget() const
{
    if (m_dropTargetPath.empty()) {
        // SHGetFolderPath is deprecated in vista, but use it for xp support.
        char desktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_DESKTOP, nullptr, 0, desktopPath))) {
            m_dropTargetPath = std::string(desktopPath);
            LOG((CLOG_INFO "using desktop for drop target: %s", m_dropTargetPath.c_str()));
        }
        else {
            LOG((CLOG_ERR "failed to get desktop path, no drop target available, error=%d", GetLastError()));
        }
    }
    return m_dropTargetPath;
}

void
MSWindowsScreen::setDropTarget(const std::string& target)
{
    m_dropTargetPath = target;
}

bool
MSWindowsScreen::isModifierRepeat(KeyModifierMask oldState, KeyModifierMask state, WPARAM wParam) const
{
    bool result = false;

    if (oldState == state && state != 0) {
        UINT virtKey = (wParam >> 16) & 0xffu;
        if ((state & KeyModifierShift) != 0
            && (virtKey == VK_LSHIFT || virtKey == VK_RSHIFT)) {
            result = true;
        }
        if ((state & KeyModifierControl) != 0
            && (virtKey == VK_LCONTROL || virtKey == VK_RCONTROL)) {
            result = true;
        }
        if ((state & KeyModifierAlt) != 0
            && (virtKey == VK_LMENU || virtKey == VK_RMENU)) {
            result = true;
        }
        if ((state & KeyModifierSuper) != 0
            && (virtKey == VK_LWIN || virtKey == VK_RWIN)) {
            result = true;
        }
    }

    return result;
}

} // namespace inputleap
