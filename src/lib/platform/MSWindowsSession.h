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

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Tlhelp32.h>

class MSWindowsSession {
public:
    MSWindowsSession();
    ~MSWindowsSession();

    //!
    /*!
    Returns true if the session ID has changed since updateActiveSession was called.
    */
    BOOL hasChanged();

    bool isProcessInSession(const char* name, PHANDLE process);

    HANDLE getUserToken(LPSECURITY_ATTRIBUTES security);

    DWORD getActiveSessionId() { return m_activeSessionId; }

    void updateActiveSession();

    std::string getActiveDesktopName();

private:
    BOOL nextProcessEntry(HANDLE snapshot, LPPROCESSENTRY32 entry);

private:
    DWORD m_activeSessionId;
};
