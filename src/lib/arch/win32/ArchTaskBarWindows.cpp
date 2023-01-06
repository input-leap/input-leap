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

#include "arch/win32/ArchTaskBarWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/IArchTaskBarReceiver.h"
#include "arch/Arch.h"
#include "arch/XArch.h"
#include "inputleap/win32/AppUtilWindows.h"

#include <string.h>
#include <shellapi.h>

static const UINT        kAddReceiver     = WM_USER + 10;
static const UINT        kRemoveReceiver  = WM_USER + 11;
static const UINT        kUpdateReceiver  = WM_USER + 12;
static const UINT        kNotifyReceiver  = WM_USER + 13;
static const UINT        kFirstReceiverID = WM_USER + 14;

//
// ArchTaskBarWindows
//

ArchTaskBarWindows* ArchTaskBarWindows::s_instance = nullptr;

ArchTaskBarWindows::ArchTaskBarWindows() :
    m_ready(false),
    m_result(0),
    m_thread(nullptr),
    m_hwnd(nullptr),
    m_taskBarRestart(0),
    m_nextID(kFirstReceiverID)
{
    // save the singleton instance
    s_instance    = this;
}

ArchTaskBarWindows::~ArchTaskBarWindows()
{
    if (m_thread != nullptr) {
        PostMessage(m_hwnd, WM_QUIT, 0, 0);
        ARCH->wait(m_thread, -1.0);
        ARCH->closeThread(m_thread);
    }
    s_instance = nullptr;
}

void
ArchTaskBarWindows::init()
{
    // and a condition variable which uses the above mutex
    m_ready       = false;

    // we're going to want to get a result from the thread we're
    // about to create to know if it initialized successfully.
    // so we lock the condition variable.
    std::unique_lock<std::mutex> lock(mutex_);

    // open a window and run an event loop in a separate thread.
    // this has to happen in a separate thread because if we
    // create a window on the current desktop with the current
    // thread then the current thread won't be able to switch
    // desktops if it needs to.
    m_thread = ARCH->newThread([this]() { threadMainLoop(); });

    // wait for child thread
    while (!m_ready) {
        ARCH->wait_cond_var(cond_var_, lock, -1.0);
    }

}

void
ArchTaskBarWindows::addDialog(HWND hwnd)
{
    ArchMiscWindows::addDialog(hwnd);
}

void
ArchTaskBarWindows::removeDialog(HWND hwnd)
{
    ArchMiscWindows::removeDialog(hwnd);
}

void
ArchTaskBarWindows::addReceiver(IArchTaskBarReceiver* receiver)
{
    // ignore bogus receiver
    if (receiver == nullptr) {
        return;
    }

    // add receiver if necessary
    ReceiverToInfoMap::iterator index = m_receivers.find(receiver);
    if (index == m_receivers.end()) {
        // add it, creating a new message ID for it
        ReceiverInfo info;
        info.m_id = getNextID();
        index = m_receivers.insert(std::make_pair(receiver, info)).first;

        // add ID to receiver mapping
        m_idTable.insert(std::make_pair(info.m_id, index));
    }

    // add receiver
    PostMessage(m_hwnd, kAddReceiver, index->second.m_id, 0);
}

void
ArchTaskBarWindows::removeReceiver(IArchTaskBarReceiver* receiver)
{
    // find receiver
    ReceiverToInfoMap::iterator index = m_receivers.find(receiver);
    if (index == m_receivers.end()) {
        return;
    }

    // remove icon.  wait for this to finish before returning.
    SendMessage(m_hwnd, kRemoveReceiver, index->second.m_id, 0);

    // recycle the ID
    recycleID(index->second.m_id);

    // discard
    m_idTable.erase(index->second.m_id);
    m_receivers.erase(index);
}

void
ArchTaskBarWindows::updateReceiver(IArchTaskBarReceiver* receiver)
{
    // find receiver
    ReceiverToInfoMap::const_iterator index = m_receivers.find(receiver);
    if (index == m_receivers.end()) {
        return;
    }

    // update icon and tool tip
    PostMessage(m_hwnd, kUpdateReceiver, index->second.m_id, 0);
}

UINT
ArchTaskBarWindows::getNextID()
{
    if (m_oldIDs.empty()) {
        return m_nextID++;
    }
    UINT id = m_oldIDs.back();
    m_oldIDs.pop_back();
    return id;
}

void
ArchTaskBarWindows::recycleID(UINT id)
{
    m_oldIDs.push_back(id);
}

void
ArchTaskBarWindows::addIcon(UINT id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CIDToReceiverMap::const_iterator index = m_idTable.find(id);
    if (index != m_idTable.end()) {
        modifyIconNoLock(index->second, NIM_ADD);
    }
}

void
ArchTaskBarWindows::removeIcon(UINT id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    removeIconNoLock(id);
}

void
ArchTaskBarWindows::updateIcon(UINT id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CIDToReceiverMap::const_iterator index = m_idTable.find(id);
    if (index != m_idTable.end()) {
        modifyIconNoLock(index->second, NIM_MODIFY);
    }
}

void
ArchTaskBarWindows::addAllIcons()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (ReceiverToInfoMap::const_iterator index = m_receivers.begin();
                                    index != m_receivers.end(); ++index) {
        modifyIconNoLock(index, NIM_ADD);
    }
}

void
ArchTaskBarWindows::removeAllIcons()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (ReceiverToInfoMap::const_iterator index = m_receivers.begin();
                                    index != m_receivers.end(); ++index) {
        removeIconNoLock(index->second.m_id);
    }
}

void
ArchTaskBarWindows::modifyIconNoLock(
                ReceiverToInfoMap::const_iterator index, DWORD taskBarMessage)
{
    // get receiver
    UINT id                        = index->second.m_id;
    IArchTaskBarReceiver* receiver = index->first;

    // lock receiver so icon and tool tip are guaranteed to be consistent
    receiver->lock();

    // get icon data
    HICON icon = static_cast<HICON>(
                const_cast<IArchTaskBarReceiver::Icon>(receiver->getIcon()));

    // get tool tip
    std::string toolTip = receiver->getToolTip();

    // done querying
    receiver->unlock();

    // prepare to add icon
    NOTIFYICONDATA data;
    data.cbSize           = sizeof(NOTIFYICONDATA);
    data.hWnd             = m_hwnd;
    data.uID              = id;
    data.uFlags           = NIF_MESSAGE;
    data.uCallbackMessage = kNotifyReceiver;
    data.hIcon            = icon;
    if (icon != nullptr) {
        data.uFlags |= NIF_ICON;
    }
    if (!toolTip.empty()) {
        strncpy(data.szTip, toolTip.c_str(), sizeof(data.szTip));
        data.szTip[sizeof(data.szTip) - 1] = '\0';
        data.uFlags                       |= NIF_TIP;
    }
    else {
        data.szTip[0] = '\0';
    }

    // add icon
    if (Shell_NotifyIcon(taskBarMessage, &data) == 0) {
        // failed
    }
}

void
ArchTaskBarWindows::removeIconNoLock(UINT id)
{
    NOTIFYICONDATA data;
    data.cbSize = sizeof(NOTIFYICONDATA);
    data.hWnd   = m_hwnd;
    data.uID    = id;
    if (Shell_NotifyIcon(NIM_DELETE, &data) == 0) {
        // failed
    }
}

void
ArchTaskBarWindows::handleIconMessage(
                IArchTaskBarReceiver* receiver, LPARAM lParam)
{
    // process message
    switch (lParam) {
    case WM_LBUTTONDOWN:
        receiver->showStatus();
        break;

    case WM_LBUTTONDBLCLK:
        receiver->primaryAction();
        break;

    case WM_RBUTTONUP: {
        POINT p;
        GetCursorPos(&p);
        receiver->runMenu(p.x, p.y);
        break;
    }

    case WM_MOUSEMOVE:
        // currently unused
        break;

    default:
        // unused
        break;
    }
}

bool
ArchTaskBarWindows::processDialogs(MSG* msg)
{
    // only one thread can be in this method on any particular object
    // at any given time.  that's not a problem since only our event
    // loop calls this method and there's just one of those.

    std::unique_lock<std::mutex> lock(mutex_);

    // remove removed dialogs
    m_dialogs.erase(false);

    // merge added dialogs into the dialog list
    for (Dialogs::const_iterator index = m_addedDialogs.begin();
                            index != m_addedDialogs.end(); ++index) {
        m_dialogs.insert(std::make_pair(index->first, index->second));
    }
    m_addedDialogs.clear();


    // check message against all dialogs until one handles it.
    // note that we don't hold a lock while checking because
    // the message is processed and may make calls to this
    // object.  that's okay because addDialog() and
    // removeDialog() don't change the map itself (just the
    // values of some elements).
    for (Dialogs::const_iterator index = m_dialogs.begin();
                            index != m_dialogs.end(); ++index) {
        if (index->second) {
            lock.unlock();
            if (IsDialogMessage(index->first, msg)) {
                return true;
            }
            lock.lock();
        }
    }

    return false;
}

LRESULT
ArchTaskBarWindows::wndProc(HWND hwnd,
                UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case kNotifyReceiver: {
        // lookup receiver
        CIDToReceiverMap::const_iterator index = m_idTable.find((UINT)wParam);
        if (index != m_idTable.end()) {
            IArchTaskBarReceiver* receiver = index->second->first;
            handleIconMessage(receiver, lParam);
            return 0;
        }
        break;
    }

    case kAddReceiver:
        addIcon((UINT)wParam);
        break;

    case kRemoveReceiver:
        removeIcon((UINT)wParam);
        break;

    case kUpdateReceiver:
        updateIcon((UINT)wParam);
        break;

    default:
        if (msg == m_taskBarRestart) {
            // task bar was recreated so re-add our icons
            addAllIcons();
        }
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK
ArchTaskBarWindows::staticWndProc(HWND hwnd, UINT msg,
                WPARAM wParam, LPARAM lParam)
{
    // if msg is WM_NCCREATE, extract the ArchTaskBarWindows* and put
    // it in the extra window data then forward the call.
    ArchTaskBarWindows* self = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT* createInfo;
        createInfo = reinterpret_cast<CREATESTRUCT*>(lParam);
        self       = static_cast<ArchTaskBarWindows*>(
                                                createInfo->lpCreateParams);
        SetWindowLongPtr(hwnd, 0, reinterpret_cast<LONG_PTR>(createInfo->lpCreateParams));
    }
    else {
        // get the extra window data and forward the call
        LONG_PTR data = GetWindowLongPtr(hwnd, 0);
        if (data != 0) {
            self = static_cast<ArchTaskBarWindows*>(reinterpret_cast<void*>(data));
        }
    }

    // forward the message
    if (self != nullptr) {
        return self->wndProc(hwnd, msg, wParam, lParam);
    }
    else {
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}

void
ArchTaskBarWindows::threadMainLoop()
{
    // register the task bar restart message
    m_taskBarRestart        = RegisterWindowMessage(TEXT("TaskbarCreated"));

    // register a window class
    LPCTSTR className = TEXT("BarrierTaskBar");
    WNDCLASSEX classInfo;
    classInfo.cbSize        = sizeof(classInfo);
    classInfo.style         = CS_NOCLOSE;
    classInfo.lpfnWndProc   = &ArchTaskBarWindows::staticWndProc;
    classInfo.cbClsExtra    = 0;
    classInfo.cbWndExtra    = sizeof(ArchTaskBarWindows*);
    classInfo.hInstance     = instanceWin32();
    classInfo.hIcon = nullptr;
    classInfo.hCursor = nullptr;
    classInfo.hbrBackground = nullptr;
    classInfo.lpszMenuName = nullptr;
    classInfo.lpszClassName = className;
    classInfo.hIconSm = nullptr;
    ATOM windowClass        = RegisterClassEx(&classInfo);

    // create window
    m_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
                            className,
                            TEXT("Barrier Task Bar"),
                            WS_POPUP,
                            0, 0, 1, 1,
                            nullptr,
                            nullptr,
                            instanceWin32(),
                            static_cast<void*>(this));

    // signal ready
    {
        std::lock_guard<std::mutex> lock(mutex_);
        m_ready = true;
        cond_var_.notify_all();
    }

    // handle failure
    if (m_hwnd == nullptr) {
        UnregisterClass(className, instanceWin32());
        return;
    }

    // main loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!processDialogs(&msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    // clean up
    removeAllIcons();
    DestroyWindow(m_hwnd);
    UnregisterClass(className, instanceWin32());
}

HINSTANCE ArchTaskBarWindows::instanceWin32()
{
    return ArchMiscWindows::instanceWin32();
}
