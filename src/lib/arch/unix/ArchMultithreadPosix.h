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

#include "config.h"

#include "arch/IArchMultithread.h"
#include "common/stdlist.h"

#include <pthread.h>
#include <mutex>

#define ARCH_MULTITHREAD ArchMultithreadPosix

//! Posix implementation of IArchMultithread
class ArchMultithreadPosix : public IArchMultithread {
public:
    ArchMultithreadPosix();
    virtual ~ArchMultithreadPosix();

    //! @name manipulators
    //@{

    void setNetworkDataForCurrentThread(void*);

    //@}
    //! @name accessors
    //@{

    void* getNetworkDataForThread(ArchThread);

    static ArchMultithreadPosix* getInstance();

    //@}

    // IArchMultithread overrides
    ArchThread newThread(const std::function<void()>& func) override;
    ArchThread newCurrentThread() override;
    ArchThread copyThread(ArchThread) override;
    void closeThread(ArchThread) override;
    void cancelThread(ArchThread) override;
    void setPriorityOfThread(ArchThread, int n) override;
    void testCancelThread() override;
    bool wait(ArchThread, double timeout) override;
    bool isSameThread(ArchThread, ArchThread) override;
    bool isExitedThread(ArchThread) override;
    ThreadID getIDOfThread(ArchThread) override;
    void setSignalHandler(ESignal, SignalFunc, void*) override;
    void raiseSignal(ESignal) override;

private:
    void startSignalHandler();

    ArchThreadImpl* find(pthread_t thread);
    ArchThreadImpl* findNoRef(pthread_t thread);
    void insert(ArchThreadImpl* thread);
    void erase(ArchThreadImpl* thread);

    void refThread(ArchThreadImpl* rep);
    void testCancelThreadImpl(ArchThreadImpl* rep);

    void doThreadFunc(ArchThread thread);
    static void* threadFunc(void* vrep);
    static void threadCancel(int);
    static void* threadSignalHandler(void* vrep);

private:
    typedef std::list<ArchThread> ThreadList;

    static ArchMultithreadPosix*    s_instance;

    bool m_newThreadCalled;

    std::mutex m_threadMutex;
    ArchThread m_mainThread;
    ThreadList m_threadList;
    ThreadID m_nextID;

    pthread_t m_signalThread;
    SignalFunc m_signalFunc[kNUM_SIGNALS];
    void* m_signalUserData[kNUM_SIGNALS];
};
