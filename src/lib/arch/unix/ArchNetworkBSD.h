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

#include "arch/IArchNetwork.h"
#include "arch/IArchMultithread.h"
#include <mutex>

#if HAVE_SYS_SOCKET_H
#    include <sys/socket.h>
#endif


// old systems may use char* for [gs]etsockopt()'s optval argument.
// this should be void on modern systems but char is forwards
// compatible so we always use it.
typedef char optval_t;

#define ARCH_NETWORK ArchNetworkBSD
#define TYPED_ADDR(type_, addr_) (reinterpret_cast<type_*>(&addr_->m_addr))

class ArchSocketImpl {
public:
    int m_fd;
    int m_refCount;
};

class ArchNetAddressImpl {
public:
    ArchNetAddressImpl() : m_len(sizeof(m_addr)) { }

public:
    struct sockaddr_storage m_addr;
    socklen_t m_len;
};

//! Berkeley (BSD) sockets implementation of IArchNetwork
class ArchNetworkBSD : public IArchNetwork {
public:
    ArchNetworkBSD();
    ~ArchNetworkBSD() override;

    void init() override;

    // IArchNetwork overrides
    ArchSocket newSocket(EAddressFamily, ESocketType) override;
    ArchSocket copySocket(ArchSocket s) override;
    void closeSocket(ArchSocket s) override;
    void closeSocketForRead(ArchSocket s) override;
    void closeSocketForWrite(ArchSocket s) override;
    void bindSocket(ArchSocket s, ArchNetAddress addr) override;
    void listenOnSocket(ArchSocket s) override;
    ArchSocket acceptSocket(ArchSocket s, ArchNetAddress* addr) override;
    bool connectSocket(ArchSocket s, ArchNetAddress name) override;
    int pollSocket(PollEntry[], int num, double timeout) override;
    void unblockPollSocket(ArchThread thread) override;
    size_t readSocket(ArchSocket s, void* buf, size_t len) override;
    size_t writeSocket(ArchSocket s, const void* buf, size_t len) override;
    void throwErrorOnSocket(ArchSocket) override;
    bool setNoDelayOnSocket(ArchSocket, bool noDelay) override;
    bool setReuseAddrOnSocket(ArchSocket, bool reuse) override;
    std::string getHostName() override;
    ArchNetAddress newAnyAddr(EAddressFamily) override;
    ArchNetAddress copyAddr(ArchNetAddress) override;
    ArchNetAddress nameToAddr(const std::string&) override;
    void closeAddr(ArchNetAddress) override;
    std::string addrToName(ArchNetAddress) override;
    std::string addrToString(ArchNetAddress) override;
    EAddressFamily getAddrFamily(ArchNetAddress) override;
    void setAddrPort(ArchNetAddress, int port) override;
    int getAddrPort(ArchNetAddress) override;
    bool isAnyAddr(ArchNetAddress) override;
    bool isEqualAddr(ArchNetAddress, ArchNetAddress) override;

private:
    const int* getUnblockPipe();
    const int* getUnblockPipeForThread(ArchThread);
    void setBlockingOnSocket(int fd, bool blocking);
    void throwError(int);
    void throwNameError(int);

private:
    std::mutex mutex_;
};
