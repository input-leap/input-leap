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

#include "arch/unix/ArchMultithreadPosix.h"

#include "arch/Arch.h"
#include "arch/XArch.h"
#include "base/Time.h"

#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <cerrno>
#include <csignal>
#include <fstream>

#define SIGWAKEUP SIGUSR1

static
void
setSignalSet(sigset_t* sigset)
{
    sigemptyset(sigset);
    sigaddset(sigset, SIGHUP);
    sigaddset(sigset, SIGINT);
    sigaddset(sigset, SIGTERM);
    sigaddset(sigset, SIGUSR2);
}

namespace inputleap {

namespace {

#ifdef __linux__

bool is_under_debugger()
{
    std::ifstream file("/proc/self/status");
    if (!file.is_open()) {
        return false;
    }

    std::string heading;
    std::string tmp;
    while (file >> heading) {
        if (heading == "TracerPid:") {
            unsigned pid = 0;
            file >> pid;
            return pid != 0;
        }
        // skip the remainder of the line
        std::getline(file, tmp);
    }

    return false;
}

#endif

} // namespace

class ArchThreadImpl {
public:
    ArchThreadImpl();

public:
    int                    m_refCount;
    IArchMultithread::ThreadID        m_id;
    pthread_t            m_thread;
    std::function<void()> func_;
    bool                m_cancel;
    bool                m_cancelling;
    bool                m_exited;
    void*                m_networkData;
};

ArchThreadImpl::ArchThreadImpl() :
    m_refCount(1),
    m_id(0),
    m_cancel(false),
    m_cancelling(false),
    m_exited(false),
    m_networkData(nullptr)
{
    // do nothing
}


//
// ArchMultithreadPosix
//

ArchMultithreadPosix* ArchMultithreadPosix::s_instance = nullptr;

ArchMultithreadPosix::ArchMultithreadPosix() :
    m_newThreadCalled(false),
    m_nextID(0)
{
    assert(s_instance == nullptr);

    s_instance = this;

    // no signal handlers
    for (size_t i = 0; i < kNUM_SIGNALS; ++i) {
        m_signalFunc[i] = nullptr;
        m_signalUserData[i] = nullptr;
    }

    // create thread for calling (main) thread and add it to our
    // list.  no need to lock the mutex since we're the only thread.
    m_mainThread           = new ArchThreadImpl;
    m_mainThread->m_thread = pthread_self();
    insert(m_mainThread);

    // install SIGWAKEUP handler.  this causes SIGWAKEUP to interrupt
    // system calls.  we use that when cancelling a thread to force it
    // to wake up immediately if it's blocked in a system call.  we
    // won't need this until another thread is created but it's fine
    // to install it now.
    struct sigaction act;
    sigemptyset(&act.sa_mask);
# if defined(SA_INTERRUPT)
    act.sa_flags   = SA_INTERRUPT;
# else
    act.sa_flags   = 0;
# endif
    act.sa_handler = &threadCancel;
    sigaction(SIGWAKEUP, &act, nullptr);

    // set desired signal dispositions.  let SIGWAKEUP through but
    // ignore SIGPIPE (we'll handle EPIPE).
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGWAKEUP);
    pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &sigset, nullptr);
}

ArchMultithreadPosix::~ArchMultithreadPosix()
{
    assert(s_instance != nullptr);

    s_instance = nullptr;
}

void
ArchMultithreadPosix::setNetworkDataForCurrentThread(void* data)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    ArchThreadImpl* thread = find(pthread_self());
    thread->m_networkData = data;
}

void*
ArchMultithreadPosix::getNetworkDataForThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    return thread->m_networkData;
}

ArchMultithreadPosix*
ArchMultithreadPosix::getInstance()
{
    return s_instance;
}

ArchThread ArchMultithreadPosix::newThread(const std::function<void()>& func)
{
    // initialize signal handler.  we do this here instead of the
    // constructor so we can avoid daemonizing (using fork())
    // when there are multiple threads.  clients can safely
    // use condition variables and mutexes before creating a
    // new thread and they can safely use the only thread
    // they have access to, the main thread, so they really
    // can't tell the difference.
    if (!m_newThreadCalled) {
        m_newThreadCalled = true;
        startSignalHandler();
    }

    // note that the child thread will wait until we release this mutex
    std::lock_guard<std::mutex> lock(m_threadMutex);

    // create thread impl for new thread
    ArchThreadImpl* thread = new ArchThreadImpl;
    thread->func_ = func;

    // create the thread.  pthread_create() on RedHat 7.2 smp fails
    // if passed a nullptr attr so use a default attr.
    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status == 0) {
        status = pthread_create(&thread->m_thread, &attr,
                            &ArchMultithreadPosix::threadFunc, thread);
        pthread_attr_destroy(&attr);
    }

    // check if thread was started
    if (status != 0) {
        // failed to start thread so clean up
        delete thread;
        thread = nullptr;
    }
    else {
        // add thread to list
        insert(thread);

        // increment ref count to account for the thread itself
        refThread(thread);
    }

    return thread;
}

ArchThread
ArchMultithreadPosix::newCurrentThread()
{
    std::lock_guard<std::mutex> lock(m_threadMutex);

    ArchThreadImpl* thread = find(pthread_self());
    assert(thread != nullptr);
    return thread;
}

void
ArchMultithreadPosix::closeThread(ArchThread thread)
{
    assert(thread != nullptr);

    // decrement ref count and clean up thread if no more references
    if (--thread->m_refCount == 0) {
        // detach from thread (unless it's the main thread)
        if (thread->func_) {
            pthread_detach(thread->m_thread);
        }

        // remove thread from list
        {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            assert(findNoRef(thread->m_thread) == thread);
            erase(thread);
        }

        // done with thread
        delete thread;
    }
}

ArchThread
ArchMultithreadPosix::copyThread(ArchThread thread)
{
    refThread(thread);
    return thread;
}

void
ArchMultithreadPosix::cancelThread(ArchThread thread)
{
    assert(thread != nullptr);

    // set cancel and wakeup flags if thread can be cancelled
    bool wakeup = false;

    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        if (!thread->m_exited && !thread->m_cancelling) {
            thread->m_cancel = true;
            wakeup = true;
        }
    }

    // force thread to exit system calls if wakeup is true
    if (wakeup) {
        pthread_kill(thread->m_thread, SIGWAKEUP);
    }
}

void
ArchMultithreadPosix::setPriorityOfThread(ArchThread thread, int /*n*/)
{
    assert(thread != nullptr);

    // FIXME
}

void
ArchMultithreadPosix::testCancelThread()
{
    // find current thread
    ArchThreadImpl* thread = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        thread = findNoRef(pthread_self());
    }

    // test cancel on thread
    testCancelThreadImpl(thread);
}

bool
ArchMultithreadPosix::wait(ArchThread target, double timeout)
{
    assert(target != nullptr);

    ArchThreadImpl* self = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_threadMutex);

        // find current thread
        self = findNoRef(pthread_self());

        // ignore wait if trying to wait on ourself
        if (target == self) {
            return false;
        }

        // ref the target so it can't go away while we're watching it
        refThread(target);
    }

    try {
        // do first test regardless of timeout
        testCancelThreadImpl(self);
        if (isExitedThread(target)) {
            closeThread(target);
            return true;
        }

        // wait and repeat test if there's a timeout
        if (timeout != 0.0) {
            const double start = inputleap::current_time_seconds();
            do {
                inputleap::this_thread_sleep(0.05);

                // repeat test
                testCancelThreadImpl(self);
                if (isExitedThread(target)) {
                    closeThread(target);
                    return true;
                }

                // repeat wait and test until timed out
            } while (timeout < 0.0 || (inputleap::current_time_seconds() - start) <= timeout);
        }

        closeThread(target);
        return false;
    }
    catch (...) {
        closeThread(target);
        throw;
    }
}

bool
ArchMultithreadPosix::isSameThread(ArchThread thread1, ArchThread thread2)
{
    return (thread1 == thread2);
}

bool
ArchMultithreadPosix::isExitedThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    return thread->m_exited;
}

IArchMultithread::ThreadID
ArchMultithreadPosix::getIDOfThread(ArchThread thread)
{
    return thread->m_id;
}

void
ArchMultithreadPosix::setSignalHandler(
                ESignal signal, SignalFunc func, void* userData)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    m_signalFunc[signal]     = func;
    m_signalUserData[signal] = userData;
}

void
ArchMultithreadPosix::raiseSignal(ESignal signal)
{
    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_signalFunc[signal] != nullptr) {
        m_signalFunc[signal](signal, m_signalUserData[signal]);
        pthread_kill(m_mainThread->m_thread, SIGWAKEUP);
    }
    else if (signal == kINTERRUPT || signal == kTERMINATE) {
        ARCH->cancelThread(m_mainThread);
    }
}

void
ArchMultithreadPosix::startSignalHandler()
{
    // set signal mask.  the main thread blocks these signals and
    // the signal handler thread will listen for them.
    sigset_t sigset, oldsigset;
    setSignalSet(&sigset);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldsigset);

    // fire up the INT and TERM signal handler thread.  we could
    // instead arrange to catch and handle these signals but
    // we'd be unable to cancel the main thread since no pthread
    // calls are allowed in a signal handler.
    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status == 0) {
        status = pthread_create(&m_signalThread, &attr, &ArchMultithreadPosix::threadSignalHandler,
                                nullptr);
        pthread_attr_destroy(&attr);
    }
    if (status != 0) {
        // can't create thread to wait for signal so don't block
        // the signals.
        pthread_sigmask(SIG_UNBLOCK, &oldsigset, nullptr);
    }
}

ArchThreadImpl*
ArchMultithreadPosix::find(pthread_t thread)
{
    ArchThreadImpl* impl = findNoRef(thread);
    if (impl != nullptr) {
        refThread(impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadPosix::findNoRef(pthread_t thread)
{
    // linear search
    for (ThreadList::const_iterator index  = m_threadList.begin();
                                     index != m_threadList.end(); ++index) {
        if ((*index)->m_thread == thread) {
            return *index;
        }
    }
    return nullptr;
}

void
ArchMultithreadPosix::insert(ArchThreadImpl* thread)
{
    assert(thread != nullptr);

    // thread shouldn't already be on the list
    assert(findNoRef(thread->m_thread) == nullptr);

    // set thread id.  note that we don't worry about m_nextID
    // wrapping back to 0 and duplicating thread ID's since the
    // likelihood of barrier running that long is vanishingly
    // small.
    thread->m_id = ++m_nextID;

    // append to list
    m_threadList.push_back(thread);
}

void
ArchMultithreadPosix::erase(ArchThreadImpl* thread)
{
    for (ThreadList::iterator index  = m_threadList.begin();
                               index != m_threadList.end(); ++index) {
        if (*index == thread) {
            m_threadList.erase(index);
            break;
        }
    }
}

void
ArchMultithreadPosix::refThread(ArchThreadImpl* thread)
{
    assert(thread != nullptr);
    assert(findNoRef(thread->m_thread) != nullptr);
    ++thread->m_refCount;
}

void
ArchMultithreadPosix::testCancelThreadImpl(ArchThreadImpl* thread)
{
    assert(thread != nullptr);

    std::lock_guard<std::mutex> lock(m_threadMutex);

    // update cancel state
    if (thread->m_cancel && !thread->m_cancelling) {
        thread->m_cancelling = true;
        thread->m_cancel     = false;

        // unwind thread's stack if cancelling
        throw XThreadCancel();
    }
}

void*
ArchMultithreadPosix::threadFunc(void* vrep)
{
    // get the thread
    ArchThreadImpl* thread = static_cast<ArchThreadImpl*>(vrep);

    // setup pthreads
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    // run thread
    s_instance->doThreadFunc(thread);

    // terminate the thread
    return nullptr;
}

void
ArchMultithreadPosix::doThreadFunc(ArchThread thread)
{
    // default priority is slightly below normal
    setPriorityOfThread(thread, 1);

    // wait for parent to initialize this object
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
    }

    try {
        thread->func_();
    }

    catch (XThreadCancel&) {
        // client called cancel()
    }
    catch (...) {
        // note -- don't catch (...) to avoid masking bugs
        {
            std::lock_guard<std::mutex> lock(m_threadMutex);
            thread->m_exited = true;
        }
        closeThread(thread);
        throw;
    }

    // thread has exited
    {
        std::lock_guard<std::mutex> lock(m_threadMutex);
        thread->m_exited = true;
    }

    // done with thread
    closeThread(thread);
}

void
ArchMultithreadPosix::threadCancel(int)
{
    // do nothing
}

void*
ArchMultithreadPosix::threadSignalHandler(void*)
{
    // detach
    pthread_detach(pthread_self());

    // add signal to mask
    sigset_t sigset;
    setSignalSet(&sigset);

    // also wait on SIGABRT.  on linux (others?) this thread (process)
    // will persist after all the other threads evaporate due to an
    // assert unless we wait on SIGABRT.  that means our resources (like
    // the socket we're listening on) are not released and never will be
    // until the lingering thread is killed.  i don't know why sigwait()
    // should protect the thread from being killed.  note that sigwait()
    // doesn't actually return if we receive SIGABRT and, for some
    // reason, we don't have to block SIGABRT.
    sigaddset(&sigset, SIGABRT);

    // we exit the loop via thread cancellation in sigwait()
    for (;;) {
        // wait
        int signal = 0;
        sigwait(&sigset, &signal);

        // if we get here then the signal was raised
        switch (signal) {
        case SIGINT:
#ifdef __linux__
            if (is_under_debugger()) {
                // On Linux GDB is not able to catch signals if they are received via sigwait
                // and friends as they are considered "not delivered" to the application by
                // Linux ptrace interface which is used by gdb
                // (see e.g. https://bugzilla.kernel.org/show_bug.cgi?id=9039).
                // Most signals are not a problem, but SIGINT is used by many tools to tell gdb
                // to interrupt the debugged command. Therefore when receiving SIGINT it is ignored
                // and sigtrap is raised instead.

                std::raise(SIGTRAP); // It is normal that debugger stops on this line, read above
                break;
            }
#endif
            ARCH->raiseSignal(kINTERRUPT);
            break;

        case SIGTERM:
            ARCH->raiseSignal(kTERMINATE);
            break;

        case SIGHUP:
            ARCH->raiseSignal(kHANGUP);
            break;

        case SIGUSR2:
            ARCH->raiseSignal(kUSER);
            break;

        default:
            // ignore
            break;
        }
    }

    return nullptr;
}

} // namespace inputleap
