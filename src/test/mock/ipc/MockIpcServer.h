/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#pragma once

#include "ipc/IpcServer.h"
#include "ipc/IpcMessage.h"
#include "arch/Arch.h"

#include "test/global/gmock.h"

#include <condition_variable>
#include <mutex>

using ::testing::_;
using ::testing::Invoke;

class IEventQueue;

class MockIpcServer : public IpcServer
{
public:
    MockIpcServer() {}

    ~MockIpcServer() override {}

    MOCK_METHOD0(listen, void());
    MOCK_METHOD2(send, void(const IpcMessage&, EIpcClientType));
    MOCK_CONST_METHOD1(hasClients, bool(EIpcClientType));

    void delegateToFake() {
        ON_CALL(*this, send(_, _)).WillByDefault(Invoke(this, &MockIpcServer::mockSend));
    }

    void waitForSend() {
        std::unique_lock<std::mutex> lock{send_mutex_};
        ARCH->wait_cond_var(send_cv_, lock, 5);
    }

private:
    void mockSend(const IpcMessage&, EIpcClientType) {
        std::lock_guard<std::mutex> lock(send_mutex_);
        send_cv_.notify_all();
    }

    std::condition_variable send_cv_;
    std::mutex send_mutex_;
};
