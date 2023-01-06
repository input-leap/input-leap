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

#include "arch/win32/ArchDaemonWindows.h"
#include "arch/win32/ArchMiscWindows.h"
#include "arch/win32/XArchWindows.h"
#include "arch/Arch.h"
#include "base/Time.h"
#include "common/stdvector.h"

#include <sstream>

//
// ArchDaemonWindows
//

ArchDaemonWindows*        ArchDaemonWindows::s_daemon = nullptr;

ArchDaemonWindows::ArchDaemonWindows() :
m_daemonThreadID(0)
{
    m_quitMessage = RegisterWindowMessage("BarrierDaemonExit");
}

ArchDaemonWindows::~ArchDaemonWindows()
{
    // do nothing
}

int
ArchDaemonWindows::runDaemon(RunFunc runFunc)
{
    assert(s_daemon != nullptr);
    return s_daemon->doRunDaemon(runFunc);
}

void
ArchDaemonWindows::daemonRunning(bool running)
{
    if (s_daemon != nullptr) {
        s_daemon->doDaemonRunning(running);
    }
}

UINT
ArchDaemonWindows::getDaemonQuitMessage()
{
    if (s_daemon != nullptr) {
        return s_daemon->doGetDaemonQuitMessage();
    }
    else {
        return 0;
    }
}

void
ArchDaemonWindows::daemonFailed(int result)
{
    assert(s_daemon != nullptr);
    throw XArchDaemonRunFailed(result);
}

void
ArchDaemonWindows::installDaemon(const char* name,
                const char* description,
                const char* pathname,
                const char* commandLine,
                const char* dependencies)
{
    // open service manager
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_WRITE);
    if (mgr == nullptr) {
        // can't open service manager
        throw XArchDaemonInstallFailed(new XArchEvalWindows);
    }

    // create the service
    SC_HANDLE service = CreateService(mgr,
                                      name,
                                      name,
                                      0,
                                      SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                      SERVICE_AUTO_START,
                                      SERVICE_ERROR_NORMAL,
                                      pathname,
                                      nullptr,
                                      nullptr,
                                      dependencies,
                                      nullptr,
                                      nullptr);

    if (service == nullptr) {
        // can't create service
        DWORD err = GetLastError();
        if (err != ERROR_SERVICE_EXISTS) {
            CloseServiceHandle(mgr);
            throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
        }
    }
    else {
        // done with service (but only try to close if not null)
        CloseServiceHandle(service);
    }

    // done with manager
    CloseServiceHandle(mgr);

    // open the registry key for this service
    HKEY key = openNTServicesKey();
    key      = ArchMiscWindows::addKey(key, name);
    if (key == nullptr) {
        // can't open key
        DWORD err = GetLastError();
        try {
            uninstallDaemon(name);
        }
        catch (...) {
            // ignore
        }
        throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
    }

    // set the description
    ArchMiscWindows::setValue(key, _T("Description"), description);

    // set command line
    key = ArchMiscWindows::addKey(key, _T("Parameters"));
    if (key == nullptr) {
        // can't open key
        DWORD err = GetLastError();
        ArchMiscWindows::closeKey(key);
        try {
            uninstallDaemon(name);
        }
        catch (...) {
            // ignore
        }
        throw XArchDaemonInstallFailed(new XArchEvalWindows(err));
    }
    ArchMiscWindows::setValue(key, _T("CommandLine"), commandLine);

    // done with registry
    ArchMiscWindows::closeKey(key);
}

void
ArchDaemonWindows::uninstallDaemon(const char* name)
{
    // remove parameters for this service.  ignore failures.
    HKEY key = openNTServicesKey();
    key      = ArchMiscWindows::openKey(key, name);
    if (key != nullptr) {
        ArchMiscWindows::deleteKey(key, _T("Parameters"));
        ArchMiscWindows::closeKey(key);
    }

    // open service manager
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_WRITE);
    if (mgr == nullptr) {
        // can't open service manager
        throw XArchDaemonUninstallFailed(new XArchEvalWindows);
    }

    // open the service.  oddly, you must open a service to delete it.
    SC_HANDLE service = OpenService(mgr, name, DELETE | SERVICE_STOP);
    if (service == nullptr) {
        DWORD err = GetLastError();
        CloseServiceHandle(mgr);
        if (err != ERROR_SERVICE_DOES_NOT_EXIST) {
            throw XArchDaemonUninstallFailed(new XArchEvalWindows(err));
        }
        throw XArchDaemonUninstallNotInstalled(new XArchEvalWindows(err));
    }

    // stop the service.  we don't care if we fail.
    SERVICE_STATUS status;
    ControlService(service, SERVICE_CONTROL_STOP, &status);

    // delete the service
    const bool okay = (DeleteService(service) == 0);
    const DWORD err = GetLastError();

    // clean up
    CloseServiceHandle(service);
    CloseServiceHandle(mgr);

    // give windows a chance to remove the service before
    // we check if it still exists.
    inputleap::this_thread_sleep(1);

    // handle failure.  ignore error if service isn't installed anymore.
    if (!okay && isDaemonInstalled(name)) {
        if (err == ERROR_SUCCESS) {
            // this seems to occur even though the uninstall was successful.
            // it could be a timing issue, i.e., isDaemonInstalled is
            // called too soon. i've added a sleep to try and stop this.
            return;
        }
        if (err == ERROR_IO_PENDING) {
            // this seems to be a spurious error
            return;
        }
        if (err != ERROR_SERVICE_MARKED_FOR_DELETE) {
            throw XArchDaemonUninstallFailed(new XArchEvalWindows(err));
        }
        throw XArchDaemonUninstallNotInstalled(new XArchEvalWindows(err));
    }
}

int
ArchDaemonWindows::daemonize(const char* name, DaemonFunc func)
{
    assert(name != nullptr);
    assert(func != nullptr);

    // save daemon function
    m_daemonFunc = func;

    // construct the service entry
    SERVICE_TABLE_ENTRY entry[2];
    entry[0].lpServiceName = const_cast<char*>(name);
    entry[0].lpServiceProc = &ArchDaemonWindows::serviceMainEntry;
    entry[1].lpServiceName = nullptr;
    entry[1].lpServiceProc = nullptr;

    // hook us up to the service control manager.  this won't return
    // (if successful) until the processes have terminated.
    s_daemon = this;
    if (StartServiceCtrlDispatcher(entry) == 0) {
        // StartServiceCtrlDispatcher failed
        s_daemon = nullptr;
        throw XArchDaemonFailed(new XArchEvalWindows);
    }

    s_daemon = nullptr;
    return m_daemonResult;
}

bool
ArchDaemonWindows::canInstallDaemon(const char* /*name*/)
{
    // check if we can open service manager for write
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_WRITE);
    if (mgr == nullptr) {
        return false;
    }
    CloseServiceHandle(mgr);

    // check if we can open the registry key
    HKEY key = openNTServicesKey();
    ArchMiscWindows::closeKey(key);

    return (key != nullptr);
}

bool
ArchDaemonWindows::isDaemonInstalled(const char* name)
{
    // open service manager
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_READ);
    if (mgr == nullptr) {
        return false;
    }

    // open the service
    SC_HANDLE service = OpenService(mgr, name, GENERIC_READ);

    // clean up
    if (service != nullptr) {
        CloseServiceHandle(service);
    }
    CloseServiceHandle(mgr);

    return (service != nullptr);
}

HKEY
ArchDaemonWindows::openNTServicesKey()
{
    static const char* s_keyNames[] = {
        _T("SYSTEM"),
        _T("CurrentControlSet"),
        _T("Services"),
        nullptr
    };

    return ArchMiscWindows::addKey(HKEY_LOCAL_MACHINE, s_keyNames);
}

bool
ArchDaemonWindows::isRunState(DWORD state)
{
    switch (state) {
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
    case SERVICE_RUNNING:
        return true;

    default:
        return false;
    }
}

int
ArchDaemonWindows::doRunDaemon(RunFunc run)
{
    // should only be called from DaemonFunc
    assert(run != nullptr);

    // create message queue for this thread
    MSG dummy;
    PeekMessage(&dummy, nullptr, 0, 0, PM_NOREMOVE);

    int result = 0;
    std::unique_lock<std::mutex> lock(service_mutex_);
    m_daemonThreadID = GetCurrentThreadId();
    while (m_serviceState != SERVICE_STOPPED) {
        // wait until we're told to start
        while (!isRunState(m_serviceState) &&
                m_serviceState != SERVICE_STOP_PENDING) {
            ARCH->wait_cond_var(service_cv_, lock, -1.0);
        }

        // run unless told to stop
        if (m_serviceState != SERVICE_STOP_PENDING) {
            lock.unlock();
            try {
                result = run();
            }
            catch (...) {
                lock.lock();
                setStatusError(0);
                m_serviceState = SERVICE_STOPPED;
                setStatus(m_serviceState);
                service_cv_.notify_all();
                throw;
            }
            lock.lock();
        }

        // notify of new state
        if (m_serviceState == SERVICE_PAUSE_PENDING) {
            m_serviceState = SERVICE_PAUSED;
        }
        else {
            m_serviceState = SERVICE_STOPPED;
        }
        setStatus(m_serviceState);
        service_cv_.notify_all();
    }
    return result;
}

void
ArchDaemonWindows::doDaemonRunning(bool running)
{
    std::lock_guard<std::mutex> lock(service_mutex_);
    if (running) {
        m_serviceState = SERVICE_RUNNING;
        setStatus(m_serviceState);
        service_cv_.notify_all();
    }
}

UINT
ArchDaemonWindows::doGetDaemonQuitMessage()
{
    return m_quitMessage;
}

void
ArchDaemonWindows::setStatus(DWORD state)
{
    setStatus(state, 0, 0);
}

void
ArchDaemonWindows::setStatus(DWORD state, DWORD step, DWORD waitHint)
{
    assert(s_daemon != nullptr);

    SERVICE_STATUS status;
    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS |
                                        SERVICE_INTERACTIVE_PROCESS;
    status.dwCurrentState            = state;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE |
                                        SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = NO_ERROR;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = step;
    status.dwWaitHint                = waitHint;
    SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void
ArchDaemonWindows::setStatusError(DWORD error)
{
    assert(s_daemon != nullptr);

    SERVICE_STATUS status;
    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS |
                                        SERVICE_INTERACTIVE_PROCESS;
    status.dwCurrentState            = SERVICE_STOPPED;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP |
                                        SERVICE_ACCEPT_PAUSE_CONTINUE |
                                        SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
    status.dwServiceSpecificExitCode = error;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;
    SetServiceStatus(s_daemon->m_statusHandle, &status);
}

void
ArchDaemonWindows::serviceMain(DWORD argc, LPTSTR* argvIn)
{
    typedef std::vector<LPCTSTR> ArgList;
    typedef std::vector<std::string> Arguments;
    const char** argv = const_cast<const char**>(argvIn);

    // register our service handler function
    m_statusHandle = RegisterServiceCtrlHandler(argv[0],
                                &ArchDaemonWindows::serviceHandlerEntry);
    if (m_statusHandle == 0) {
        // cannot start as service
        m_daemonResult = -1;
        return;
    }

    // tell service control manager that we're starting
    m_serviceState = SERVICE_START_PENDING;
    setStatus(m_serviceState, 0, 10000);

    std::string commandLine;

    // if no arguments supplied then try getting them from the registry.
    // the first argument doesn't count because it's the service name.
    Arguments args;
    ArgList myArgv;
    if (argc <= 1) {
        // read command line
        HKEY key = openNTServicesKey();
        key      = ArchMiscWindows::openKey(key, argvIn[0]);
        key      = ArchMiscWindows::openKey(key, _T("Parameters"));
        if (key != nullptr) {
            commandLine = ArchMiscWindows::readValueString(key,
                                                _T("CommandLine"));
        }

        // if the command line isn't empty then parse and use it
        if (!commandLine.empty()) {
            // parse, honoring double quoted substrings
            std::string::size_type i = commandLine.find_first_not_of(" \t");
            while (i != std::string::npos && i != commandLine.size()) {
                // find end of string
                std::string::size_type e;
                if (commandLine[i] == '\"') {
                    // quoted.  find closing quote.
                    ++i;
                    e = commandLine.find("\"", i);

                    // whitespace must follow closing quote
                    if (e == std::string::npos ||
                        (e + 1 != commandLine.size() &&
                        commandLine[e + 1] != ' ' &&
                        commandLine[e + 1] != '\t')) {
                        args.clear();
                        break;
                    }

                    // extract
                    args.push_back(commandLine.substr(i, e - i));
                    i = e + 1;
                }
                else {
                    // unquoted.  find next whitespace.
                    e = commandLine.find_first_of(" \t", i);
                    if (e == std::string::npos) {
                        e = commandLine.size();
                    }

                    // extract
                    args.push_back(commandLine.substr(i, e - i));
                    i = e + 1;
                }

                // next argument
                i = commandLine.find_first_not_of(" \t", i);
            }

            // service name goes first
            myArgv.push_back(argv[0]);

            // get pointers
            for (size_t j = 0; j < args.size(); ++j) {
                myArgv.push_back(args[j].c_str());
            }

            // adjust argc/argv
            argc = (DWORD)myArgv.size();
            argv = &myArgv[0];
        }
    }

    m_commandLine = commandLine;

    try {
        // invoke daemon function
        m_daemonResult = m_daemonFunc(static_cast<int>(argc), argv);
    }
    catch (XArchDaemonRunFailed& e) {
        setStatusError(e.m_result);
        m_daemonResult = -1;
    }
    catch (...) {
        setStatusError(1);
        m_daemonResult = -1;
    }

    // we're going to exit now, so set status to stopped
    m_serviceState = SERVICE_STOPPED;
    setStatus(m_serviceState, 0, 10000);
}

void WINAPI
ArchDaemonWindows::serviceMainEntry(DWORD argc, LPTSTR* argv)
{
    s_daemon->serviceMain(argc, argv);
}

void
ArchDaemonWindows::serviceHandler(DWORD ctrl)
{
    std::unique_lock<std::mutex> lock(service_mutex_);

    // ignore request if service is already stopped
    if (s_daemon == nullptr || m_serviceState == SERVICE_STOPPED) {
        if (s_daemon != nullptr) {
            setStatus(m_serviceState);
        }
        return;
    }

    switch (ctrl) {
    case SERVICE_CONTROL_PAUSE:
        m_serviceState = SERVICE_PAUSE_PENDING;
        setStatus(m_serviceState, 0, 5000);
        PostThreadMessage(m_daemonThreadID, m_quitMessage, 0, 0);
        while (isRunState(m_serviceState)) {
            ARCH->wait_cond_var(service_cv_, lock, -1.0);
        }
        break;

    case SERVICE_CONTROL_CONTINUE:
        // FIXME -- maybe should flush quit messages from queue
        m_serviceState = SERVICE_CONTINUE_PENDING;
        setStatus(m_serviceState, 0, 5000);
        service_cv_.notify_all();
        break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        m_serviceState = SERVICE_STOP_PENDING;
        setStatus(m_serviceState, 0, 5000);
        PostThreadMessage(m_daemonThreadID, m_quitMessage, 0, 0);
        service_cv_.notify_all();
        while (isRunState(m_serviceState)) {
            ARCH->wait_cond_var(service_cv_, lock, -1.0);
        }
        break;

    default:
        // unknown service command
        // fall through

    case SERVICE_CONTROL_INTERROGATE:
        setStatus(m_serviceState);
        break;
    }
}

void WINAPI
ArchDaemonWindows::serviceHandlerEntry(DWORD ctrl)
{
    s_daemon->serviceHandler(ctrl);
}

void
ArchDaemonWindows::start(const char* name)
{
    // open service manager
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_READ);
    if (mgr == nullptr) {
        throw XArchDaemonFailed(new XArchEvalWindows());
    }

    // open the service
    SC_HANDLE service = OpenService(
        mgr, name, SERVICE_START);

    if (service == nullptr) {
        CloseServiceHandle(mgr);
        throw XArchDaemonFailed(new XArchEvalWindows());
    }

    // start the service
    if (!StartService(service, 0, nullptr)) {
        throw XArchDaemonFailed(new XArchEvalWindows());
    }
}

void
ArchDaemonWindows::stop(const char* name)
{
    // open service manager
    SC_HANDLE mgr = OpenSCManager(nullptr, nullptr, GENERIC_READ);
    if (mgr == nullptr) {
        throw XArchDaemonFailed(new XArchEvalWindows());
    }

    // open the service
    SC_HANDLE service = OpenService(
        mgr, name,
        SERVICE_STOP | SERVICE_QUERY_STATUS);

    if (service == nullptr) {
        CloseServiceHandle(mgr);
        throw XArchDaemonFailed(new XArchEvalWindows());
    }

    // ask the service to stop, asynchronously
    SERVICE_STATUS ss;
    if (!ControlService(service, SERVICE_CONTROL_STOP, &ss)) {
        DWORD dwErrCode = GetLastError();
        if (dwErrCode != ERROR_SERVICE_NOT_ACTIVE) {
            throw XArchDaemonFailed(new XArchEvalWindows());
        }
    }
}

void
ArchDaemonWindows::installDaemon()
{
    // install default daemon if not already installed.
    if (!isDaemonInstalled(DEFAULT_DAEMON_NAME)) {
        char path[MAX_PATH];
        GetModuleFileName(ArchMiscWindows::instanceWin32(), path, MAX_PATH);

        // wrap in quotes so a malicious user can't start \Program.exe as admin.
        std::stringstream ss;
        ss << '"';
        ss << path;
        ss << '"';

        installDaemon(DEFAULT_DAEMON_NAME, DEFAULT_DAEMON_INFO, ss.str().c_str(), "", "");
    }

    start(DEFAULT_DAEMON_NAME);
}

void
ArchDaemonWindows::uninstallDaemon()
{
    // remove service if installed.
    if (isDaemonInstalled(DEFAULT_DAEMON_NAME)) {
        uninstallDaemon(DEFAULT_DAEMON_NAME);
    }
}
