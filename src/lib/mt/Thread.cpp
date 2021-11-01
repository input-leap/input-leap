/*
 * barrier -- mouse and keyboard sharing utility
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

#include "mt/Thread.h"

#include "mt/XMT.h"
#include "mt/XThread.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/IJob.h"

//
// Thread
//

Thread::Thread(const std::function<void()>& fun)
{
    m_thread = ARCH->newThread([=](){ threadFunc(fun); });
    if (m_thread == NULL) {
        throw XMTThreadUnavailable();
    }
}

Thread::Thread(const Thread& thread)
{
    m_thread = ARCH->copyThread(thread.m_thread);
}

Thread::Thread(ArchThread adoptedThread)
{
    m_thread = adoptedThread;
}

Thread::~Thread()
{
    ARCH->closeThread(m_thread);
}

Thread&
Thread::operator=(const Thread& thread)
{
    // copy given thread and release ours
    ArchThread copy = ARCH->copyThread(thread.m_thread);
    ARCH->closeThread(m_thread);

    // cut over
    m_thread = copy;

    return *this;
}

void
Thread::exit(void* result)
{
    throw XThreadExit();
}

void
Thread::cancel()
{
    ARCH->cancelThread(m_thread);
}

void
Thread::setPriority(int n)
{
    ARCH->setPriorityOfThread(m_thread, n);
}

void
Thread::unblockPollSocket()
{
    ARCH->unblockPollSocket(m_thread);
}

Thread
Thread::getCurrentThread()
{
    return Thread(ARCH->newCurrentThread());
}

void
Thread::testCancel()
{
    ARCH->testCancelThread();
}

bool
Thread::wait(double timeout) const
{
    return ARCH->wait(m_thread, timeout);
}

IArchMultithread::ThreadID
Thread::getID() const
{
    return ARCH->getIDOfThread(m_thread);
}

bool
Thread::operator==(const Thread& thread) const
{
    return ARCH->isSameThread(m_thread, thread.m_thread);
}

bool
Thread::operator!=(const Thread& thread) const
{
    return !ARCH->isSameThread(m_thread, thread.m_thread);
}

void Thread::threadFunc(const std::function<void()>& func)
{
    // get this thread's id for logging
    IArchMultithread::ThreadID id;
    {
        ArchThread thread = ARCH->newCurrentThread();
        id = ARCH->getIDOfThread(thread);
        ARCH->closeThread(thread);
    }

    try {
        // go
        LOG((CLOG_DEBUG1 "thread 0x%08x entry", id));
        func();
        LOG((CLOG_DEBUG1 "thread 0x%08x exit", id));
    }
    catch (XThreadCancel&) {
        // client called cancel()
        LOG((CLOG_DEBUG1 "caught cancel on thread 0x%08x", id));
        throw;
    }
    catch (XThreadExit&) {
        LOG((CLOG_DEBUG1 "caught exit on thread 0x%08x", id));
    }
    catch (XBase& e) {
        LOG((CLOG_ERR "exception on thread 0x%08x: %s", id, e.what()));
        throw;
    }
    catch (...) {
        LOG((CLOG_ERR "exception on thread 0x%08x: <unknown>", id));
        throw;
    }
}
