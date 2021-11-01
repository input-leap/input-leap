/*
 * barrier -- mouse and keyboard sharing utility
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

#include "ConnectionSecurityLevel.h"
#include "net/TCPSocket.h"
#include "net/XSocket.h"
#include "io/filesystem.h"
#include <mutex>

class IEventQueue;
class SocketMultiplexer;
class ISocketMultiplexerJob;

struct Ssl;

//! Secure socket
/*!
A secure socket using SSL.
*/
class SecureSocket : public TCPSocket {
public:
    SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer,
                 IArchNetwork::EAddressFamily family, ConnectionSecurityLevel security_level);
    SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer,
                 ArchSocket socket, ConnectionSecurityLevel security_level);
    ~SecureSocket();

    // ISocket overrides
    void                close() override;

    // IDataSocket overrides
    virtual void        connect(const NetworkAddress&) override;

    std::unique_ptr<ISocketMultiplexerJob> newJob() override;
    bool                isFatal() const override { return m_fatal; }
    void                isFatal(bool b) { m_fatal = b; }
    bool                isSecureReady();
    void                secureConnect();
    void                secureAccept();
    int                    secureRead(void* buffer, int size, int& read);
    int                    secureWrite(const void* buffer, int size, int& wrote);
    EJobResult            doRead() override;
    EJobResult            doWrite() override;
    void                initSsl(bool server);
    bool load_certificates(const barrier::fs::path& path);

private:
    // SSL
    void initContext(bool server); // may only be called with ssl_mutex_ acquired
    void createSSL(); // may only be called with ssl_mutex_ acquired.
    int                    secureAccept(int s);
    int                    secureConnect(int s);
    bool ensure_peer_certificate(); // may only be called with ssl_mutex_ acquired

    void checkResult(int n, int& retry); // may only be called with m_ssl_mutex_ acquired.

    void                showError(const std::string& reason);
    std::string getError();
    void                disconnect();

    // may only be called with ssl_mutex_ acquired
    bool verify_cert_fingerprint(const barrier::fs::path& fingerprint_db_path);

    MultiplexerJobStatus serviceConnect(ISocketMultiplexerJob*, bool, bool, bool);
    MultiplexerJobStatus serviceAccept(ISocketMultiplexerJob*, bool, bool, bool);

    void showSecureConnectInfo(); // may only be called with ssl_mutex_ acquired
    void showSecureLibInfo();
    void showSecureCipherInfo(); // may only be called with ssl_mutex_ acquired

    void                handleTCPConnected(const Event& event, void*);

    void freeSSLResources();

private:
    // all accesses to m_ssl must be protected by this mutex. The only function that is called
    // from outside SocketMultiplexer thread is close(), so we mostly care about things accessed
    // by it.
    std::mutex ssl_mutex_;

    Ssl*                m_ssl;
    bool                m_secureReady;
    bool                m_fatal;
    ConnectionSecurityLevel security_level_ = ConnectionSecurityLevel::ENCRYPTED;

    int secure_accept_retry_ = 0; // used only in secureAccept()
    int secure_connect_retry_ = 0; // used only in secureConnect()
    int secure_read_retry_ = 0; // used only in secureRead()
    int secure_write_retry_ = 0; // used only in secureWrite()

    // The following are used only from doWrite()
    // FIXME: using std::vector would simplify logic significantly.
    bool do_write_retry_ = false;
    int do_write_retry_size_ = 0;
    std::unique_ptr<char[]> do_write_retry_buffer_;
    std::size_t do_write_retry_buffer_size_ = 0;
};
