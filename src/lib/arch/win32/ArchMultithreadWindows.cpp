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

#if defined(_MSC_VER) && !defined(_MT)
#    error multithreading compile option is required
#endif

#include "arch/win32/ArchMultithreadWindows.h"
#include "arch/Arch.h"
#include "arch/XArch.h"

#include <process.h>

//
// note -- implementation of condition variable taken from:
//   http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
// titled "Strategies for Implementing POSIX Condition Variables
// on Win32."  it also provides an implementation that doesn't
// suffer from the incorrectness problem described in our
// corresponding header but it is slower, still unfair, and
// can cause busy waiting.
//

//
// ArchThreadImpl
//

class ArchThreadImpl {
public:
    ArchThreadImpl();
    ~ArchThreadImpl();

public:
    int                    m_refCount;
    HANDLE                m_thread;
    DWORD                m_id;
    std::function<void()> func_;
    HANDLE                m_cancel;
    bool                m_cancelling;
    HANDLE                m_exit;
    void*                m_networkData;
};

ArchThreadImpl::ArchThreadImpl() :
    m_refCount(1),
    m_thread(nullptr),
    m_id(0),
    m_cancelling(false),
    m_networkData(nullptr)
{
    m_exit = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    m_cancel = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

ArchThreadImpl::~ArchThreadImpl()
{
    CloseHandle(m_exit);
    CloseHandle(m_cancel);
}


//
// ArchMultithreadWindows
//

ArchMultithreadWindows* ArchMultithreadWindows::s_instance = nullptr;

ArchMultithreadWindows::ArchMultithreadWindows()
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
    m_mainThread->m_thread = nullptr;
    m_mainThread->m_id     = GetCurrentThreadId();
    insert(m_mainThread);
}

ArchMultithreadWindows::~ArchMultithreadWindows()
{
    s_instance = nullptr;

    // clean up thread list
    for (ThreadList::iterator index  = m_threadList.begin();
                               index != m_threadList.end(); ++index) {
        delete *index;
    }
}

void
ArchMultithreadWindows::setNetworkDataForCurrentThread(void* data)
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    ArchThreadImpl* thread = findNoRef(GetCurrentThreadId());
    thread->m_networkData = data;
}

void*
ArchMultithreadWindows::getNetworkDataForThread(ArchThread thread)
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    return thread->m_networkData;
}

HANDLE
ArchMultithreadWindows::getCancelEventForCurrentThread()
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    ArchThreadImpl* thread = findNoRef(GetCurrentThreadId());
    return thread->m_cancel;
}

ArchMultithreadWindows*
ArchMultithreadWindows::getInstance()
{
    return s_instance;
}

ArchThread ArchMultithreadWindows::newThread(const std::function<void()>& func)
{
    // note that the child thread will wait until we release this mutex
    std::lock_guard<std::mutex> lock(thread_mutex_);

    // create thread impl for new thread
    ArchThreadImpl* thread = new ArchThreadImpl;
    thread->func_ = func;

    // create thread
    unsigned int id = 0;
    thread->m_thread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, threadFunc, (void*)thread, 0, &id));
    thread->m_id     = static_cast<DWORD>(id);

    // check if thread was started
    if (thread->m_thread == 0) {
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
ArchMultithreadWindows::newCurrentThread()
{
    std::lock_guard<std::mutex> lock(thread_mutex_);

    ArchThreadImpl* thread = find(GetCurrentThreadId());
    assert(thread != nullptr);
    return thread;
}

void
ArchMultithreadWindows::closeThread(ArchThread thread)
{
    assert(thread != nullptr);

    // decrement ref count and clean up thread if no more references
    if (--thread->m_refCount == 0) {
        // close the handle (main thread has a nullptr handle)
        if (thread->m_thread != nullptr) {
            CloseHandle(thread->m_thread);
        }

        // remove thread from list
        std::lock_guard<std::mutex> lock(thread_mutex_);
        assert(findNoRefOrCreate(thread->m_id) == thread);
        erase(thread);
        delete thread;
    }
}

ArchThread
ArchMultithreadWindows::copyThread(ArchThread thread)
{
    refThread(thread);
    return thread;
}

void
ArchMultithreadWindows::cancelThread(ArchThread thread)
{
    assert(thread != nullptr);

    // set cancel flag
    SetEvent(thread->m_cancel);
}

void
ArchMultithreadWindows::setPriorityOfThread(ArchThread thread, int n)
{
    struct PriorityInfo {
    public:
        DWORD        m_class;
        int            m_level;
    };
    static const PriorityInfo s_pClass[] = {
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_IDLE         },
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_LOWEST       },
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_BELOW_NORMAL },
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_NORMAL       },
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_ABOVE_NORMAL },
        { IDLE_PRIORITY_CLASS,     THREAD_PRIORITY_HIGHEST      },
        { NORMAL_PRIORITY_CLASS,   THREAD_PRIORITY_LOWEST       },
        { NORMAL_PRIORITY_CLASS,   THREAD_PRIORITY_BELOW_NORMAL },
        { NORMAL_PRIORITY_CLASS,   THREAD_PRIORITY_NORMAL       },
        { NORMAL_PRIORITY_CLASS,   THREAD_PRIORITY_ABOVE_NORMAL },
        { NORMAL_PRIORITY_CLASS,   THREAD_PRIORITY_HIGHEST      },
        { HIGH_PRIORITY_CLASS,     THREAD_PRIORITY_LOWEST       },
        { HIGH_PRIORITY_CLASS,     THREAD_PRIORITY_BELOW_NORMAL },
        { HIGH_PRIORITY_CLASS,     THREAD_PRIORITY_NORMAL       },
        { HIGH_PRIORITY_CLASS,     THREAD_PRIORITY_ABOVE_NORMAL },
        { HIGH_PRIORITY_CLASS,     THREAD_PRIORITY_HIGHEST      },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_IDLE         },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_LOWEST       },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_BELOW_NORMAL },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_NORMAL       },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_ABOVE_NORMAL },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_HIGHEST      },
        { REALTIME_PRIORITY_CLASS, THREAD_PRIORITY_TIME_CRITICAL}
    };
#if defined(_DEBUG)
    // don't use really high priorities when debugging
    static const size_t s_pMax  = 13;
#else
    static const size_t s_pMax  = sizeof(s_pClass) / sizeof(s_pClass[0]) - 1;
#endif
    static const size_t s_pBase = 8;    // index of normal priority

    assert(thread != nullptr);

    size_t index;
    if (n > 0 && s_pBase < (size_t)n) {
        // lowest priority
        index = 0;
    }
    else {
        index = (size_t)((int)s_pBase - n);
        if (index > s_pMax) {
            // highest priority
            index = s_pMax;
        }
    }
    SetPriorityClass(GetCurrentProcess(), s_pClass[index].m_class);
    SetThreadPriority(thread->m_thread, s_pClass[index].m_level);
}

void
ArchMultithreadWindows::testCancelThread()
{
    ArchThreadImpl* thread = nullptr;
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        thread = findNoRef(GetCurrentThreadId());
    }

    // test cancel on thread
    testCancelThreadImpl(thread);
}

bool
ArchMultithreadWindows::wait(ArchThread target, double timeout)
{
    assert(target != nullptr);

    ArchThreadImpl* self = nullptr;
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);

        // find current thread
        self = findNoRef(GetCurrentThreadId());

        // ignore wait if trying to wait on ourself
        if (target == self) {
            return false;
        }

        // ref the target so it can't go away while we're watching it
        refThread(target);
    }

    // convert timeout
    DWORD t;
    if (timeout < 0.0) {
        t = INFINITE;
    }
    else {
        t = (DWORD)(1000.0 * timeout);
    }

    // wait for this thread to be cancelled or woken up or for the
    // target thread to terminate.
    HANDLE handles[2];
    handles[0] = target->m_exit;
    handles[1] = self->m_cancel;
    DWORD result = WaitForMultipleObjects(2, handles, FALSE, t);

    // cancel takes priority
    if (result != WAIT_OBJECT_0 + 1 &&
        WaitForSingleObject(handles[1], 0) == WAIT_OBJECT_0) {
        result = WAIT_OBJECT_0 + 1;
    }

    // release target
    closeThread(target);

    // handle result
    switch (result) {
    case WAIT_OBJECT_0 + 0:
        // target thread terminated
        return true;

    case WAIT_OBJECT_0 + 1:
        // this thread was cancelled.  does not return.
        testCancelThreadImpl(self);

    default:
        // timeout or error
        return false;
    }
}

bool
ArchMultithreadWindows::isSameThread(ArchThread thread1, ArchThread thread2)
{
    return (thread1 == thread2);
}

bool
ArchMultithreadWindows::isExitedThread(ArchThread thread)
{
    // poll exit event
    return (WaitForSingleObject(thread->m_exit, 0) == WAIT_OBJECT_0);
}

IArchMultithread::ThreadID
ArchMultithreadWindows::getIDOfThread(ArchThread thread)
{
    return static_cast<ThreadID>(thread->m_id);
}

void
ArchMultithreadWindows::setSignalHandler(
                ESignal signal, SignalFunc func, void* userData)
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    m_signalFunc[signal]     = func;
    m_signalUserData[signal] = userData;
}

void
ArchMultithreadWindows::raiseSignal(ESignal signal)
{
    std::lock_guard<std::mutex> lock(thread_mutex_);
    if (m_signalFunc[signal] != nullptr) {
        m_signalFunc[signal](signal, m_signalUserData[signal]);
        ARCH->unblockPollSocket(m_mainThread);
    }
    else if (signal == kINTERRUPT || signal == kTERMINATE) {
        ARCH->cancelThread(m_mainThread);
    }
}

ArchThreadImpl*
ArchMultithreadWindows::find(DWORD id)
{
    ArchThreadImpl* impl = findNoRef(id);
    if (impl != nullptr) {
        refThread(impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadWindows::findNoRef(DWORD id)
{
    ArchThreadImpl* impl = findNoRefOrCreate(id);
    if (impl == nullptr) {
        // create thread for calling thread which isn't in our list and
        // add it to the list.  this won't normally happen but it can if
        // the system calls us under a new thread, like it does when we
        // run as a service.
        impl           = new ArchThreadImpl;
        impl->m_thread = nullptr;
        impl->m_id     = GetCurrentThreadId();
        insert(impl);
    }
    return impl;
}

ArchThreadImpl*
ArchMultithreadWindows::findNoRefOrCreate(DWORD id)
{
    // linear search
    for (ThreadList::const_iterator index  = m_threadList.begin();
                                     index != m_threadList.end(); ++index) {
        if ((*index)->m_id == id) {
            return *index;
        }
    }
    return nullptr;
}

void
ArchMultithreadWindows::insert(ArchThreadImpl* thread)
{
    assert(thread != nullptr);

    // thread shouldn't already be on the list
    assert(findNoRefOrCreate(thread->m_id) == nullptr);

    // append to list
    m_threadList.push_back(thread);
}

void
ArchMultithreadWindows::erase(ArchThreadImpl* thread)
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
ArchMultithreadWindows::refThread(ArchThreadImpl* thread)
{
    assert(thread != nullptr);
    assert(findNoRefOrCreate(thread->m_id) != nullptr);
    ++thread->m_refCount;
}

void
ArchMultithreadWindows::testCancelThreadImpl(ArchThreadImpl* thread)
{
    assert(thread != nullptr);

    // poll cancel event.  return if not set.
    const DWORD result = WaitForSingleObject(thread->m_cancel, 0);
    if (result != WAIT_OBJECT_0) {
        return;
    }

    // update cancel state
    std::lock_guard<std::mutex> lock(thread_mutex_);

    bool cancel          = !thread->m_cancelling;
    thread->m_cancelling = true;
    ResetEvent(thread->m_cancel);

    // unwind thread's stack if cancelling
    if (cancel) {
        throw XThreadCancel();
    }
}

unsigned int __stdcall
ArchMultithreadWindows::threadFunc(void* vrep)
{
    // get the thread
    ArchThreadImpl* thread = static_cast<ArchThreadImpl*>(vrep);

    // run thread
    s_instance->doThreadFunc(thread);

    // terminate the thread
    return 0;
}

void
ArchMultithreadWindows::doThreadFunc(ArchThread thread)
{
    // wait for parent to initialize this object
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
    }

    try {
        thread->func_();
    }

    catch (XThreadCancel&) {
        // client called cancel()
    }
    catch (...) {
        // note -- don't catch (...) to avoid masking bugs
        SetEvent(thread->m_exit);
        closeThread(thread);
        throw;
    }

    // thread has exited
    SetEvent(thread->m_exit);

    // done with thread
    closeThread(thread);
}
