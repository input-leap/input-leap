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

#include "platform/MSWindowsSession.h"
#include "inputleap/XBarrier.h"
#include "arch/IArchMultithread.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <list>

class Thread;
class IpcLogOutputter;
class IpcServer;
class FileLogOutputter;

class MSWindowsWatchdog {
public:
    MSWindowsWatchdog(
        bool daemonized,
        bool autoDetectCommand,
        IpcServer& ipcServer,
        IpcLogOutputter& ipcLogOutputter);

    void startAsync();
    std::string getCommand() const;
    void setCommand(const std::string& command, bool elevate);
    void stop();
    bool isProcessActive();
    void setFileLogOutputter(FileLogOutputter* outputter);

private:
    void main_loop();
    void output_loop();
    void shutdownProcess(HANDLE handle, DWORD pid, int timeout);
    void shutdownExistingProcesses();
    HANDLE duplicateProcessToken(HANDLE process, LPSECURITY_ATTRIBUTES security);
    HANDLE getUserToken(LPSECURITY_ATTRIBUTES security);
    void startProcess();
    BOOL doStartProcessAsUser(std::string& command, HANDLE userToken, LPSECURITY_ATTRIBUTES sa);
    BOOL doStartProcessAsSelf(std::string& command);

private:
    Thread* m_thread;
    bool m_autoDetectCommand;
    std::string m_command;
    bool m_monitoring;
    bool m_commandChanged;
    HANDLE m_stdOutWrite;
    HANDLE m_stdOutRead;
    Thread* m_outputThread;
    IpcServer& m_ipcServer;
    IpcLogOutputter& m_ipcLogOutputter;
    bool m_elevateProcess;
    MSWindowsSession m_session;
    PROCESS_INFORMATION m_processInfo;
    int m_processFailures;
    bool m_processRunning;
    FileLogOutputter* m_fileLogOutputter;
    bool m_autoElevated;
    bool m_daemonized;
};

//! Relauncher error
/*!
An error occurred in the process watchdog.
*/
class XMSWindowsWatchdogError : public XBarrier {
public:
    XMSWindowsWatchdogError(const std::string& msg) : XBarrier(msg) { }

    // XBase overrides
    virtual std::string getWhat() const noexcept { return what(); }
};
