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

#include "inputleap/ClientTaskBarReceiver.h"
#include "common/win32/winapi.h"

class BufferedLogOutputter;
class IEventQueue;

//! Implementation of ClientTaskBarReceiver for Microsoft Windows
class MSWindowsClientTaskBarReceiver : public ClientTaskBarReceiver {
public:
    MSWindowsClientTaskBarReceiver(HINSTANCE, const BufferedLogOutputter*, IEventQueue* events);
    virtual ~MSWindowsClientTaskBarReceiver();

    // IArchTaskBarReceiver overrides
    virtual void showStatus();
    virtual void runMenu(int x, int y);
    virtual void primaryAction();
    virtual Icon getIcon() const;
    void cleanup();

protected:
    void copyLog() const;

    // ClientTaskBarReceiver overrides
    virtual void onStatusChanged();

private:
    HICON loadIcon(UINT);
    void deleteIcon(HICON);
    void createWindow();
    void destroyWindow();

    BOOL dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static BOOL CALLBACK staticDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_appInstance;
    HWND m_window;
    HMENU m_menu;
    HICON m_icon[kMaxState];
    const BufferedLogOutputter* m_logBuffer;

    static const UINT    s_stateToIconID[];
};
