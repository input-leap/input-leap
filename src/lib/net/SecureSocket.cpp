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

#include "SecureSocket.h"
#include "SecureUtils.h"

#include "net/TSocketMultiplexerMethodJob.h"
#include "base/TMethodEventJob.h"
#include "net/TCPSocket.h"
#include "mt/Lock.h"
#include "arch/XArch.h"
#include "base/Log.h"
#include "base/String.h"
#include "common/DataDirectories.h"
#include "io/filesystem.h"
#include "net/FingerprintDatabase.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <memory>

//
// SecureSocket
//

#define MAX_ERROR_SIZE 65535

static const std::size_t MAX_INPUT_BUFFER_SIZE = 1024 * 1024;
static const float s_retryDelay = 0.01f;

enum {
    kMsgSize = 128
};

struct Ssl {
    SSL_CTX*    m_context;
    SSL*        m_ssl;
};

SecureSocket::SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer,
                           IArchNetwork::EAddressFamily family,
                           ConnectionSecurityLevel security_level) :
    TCPSocket(events, socketMultiplexer, family),
    m_ssl(nullptr),
    m_secureReady(false),
    m_fatal(false),
    security_level_{security_level}
{
}

SecureSocket::SecureSocket(IEventQueue* events, SocketMultiplexer* socketMultiplexer,
                           ArchSocket socket, ConnectionSecurityLevel security_level) :
    TCPSocket(events, socketMultiplexer, socket),
    m_ssl(nullptr),
    m_secureReady(false),
    m_fatal(false),
    security_level_{security_level}
{
}

SecureSocket::~SecureSocket()
{
    isFatal(true);
    // take socket from multiplexer ASAP otherwise the race condition
    // could cause events to get called on a dead object. TCPSocket
    // will do this, too, but the double-call is harmless
    removeJob();
    freeSSLResources();

    // removing sleep() because I have no idea why you would want to do it
    // ... smells of trying to cover up a bug you don't understand
    //ARCH->sleep(1);
    delete m_ssl;
}

void
SecureSocket::close()
{
    isFatal(true);
    freeSSLResources();
    TCPSocket::close();
}

void SecureSocket::freeSSLResources()
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    if (m_ssl->m_ssl != NULL) {
        SSL_shutdown(m_ssl->m_ssl);
        SSL_free(m_ssl->m_ssl);
        m_ssl->m_ssl = NULL;
    }

    if (m_ssl->m_context != NULL) {
        SSL_CTX_free(m_ssl->m_context);
        m_ssl->m_context = NULL;
    }
}

void
SecureSocket::connect(const NetworkAddress& addr)
{
    m_events->adoptHandler(m_events->forIDataSocket().connected(),
                getEventTarget(),
                new TMethodEventJob<SecureSocket>(this,
                        &SecureSocket::handleTCPConnected));

    TCPSocket::connect(addr);
}

std::unique_ptr<ISocketMultiplexerJob> SecureSocket::newJob()
{
    // after TCP connection is established, SecureSocket will pick up
    // connected event and do secureConnect
    if (m_connected && !m_secureReady) {
        return {};
    }

    return TCPSocket::newJob();
}

void
SecureSocket::secureConnect()
{
    setJob(std::make_unique<TSocketMultiplexerMethodJob>([this](auto j, auto r, auto w, auto e)
                                                         { return serviceConnect(j, r, w, e); },
                                                         getSocket(), isReadable(), isWritable()));
}

void
SecureSocket::secureAccept()
{
    setJob(std::make_unique<TSocketMultiplexerMethodJob>([this](auto j, auto r, auto w, auto e)
                                                         { return serviceAccept(j, r, w, e); },
                                                         getSocket(), isReadable(), isWritable()));
}

TCPSocket::EJobResult
SecureSocket::doRead()
{
    UInt8 buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = 0;
    int status = 0;

    if (isSecureReady()) {
        status = secureRead(buffer, sizeof(buffer), bytesRead);
        if (status < 0) {
            return kBreak;
        }
        else if (status == 0) {
            return kNew;
        }
    }
    else {
        return kRetry;
    }

    if (bytesRead > 0) {
        bool wasEmpty = (m_inputBuffer.getSize() == 0);

        // slurp up as much as possible
        do {
            m_inputBuffer.write(buffer, bytesRead);

            if (m_inputBuffer.getSize() > MAX_INPUT_BUFFER_SIZE) {
                break;
            }

            status = secureRead(buffer, sizeof(buffer), bytesRead);
            if (status < 0) {
                return kBreak;
            }
        } while (bytesRead > 0 || status > 0);

        // send input ready if input buffer was empty
        if (wasEmpty) {
            sendEvent(m_events->forIStream().inputReady());
        }
    }
    else {
        // remote write end of stream hungup.  our input side
        // has therefore shutdown but don't flush our buffer
        // since there's still data to be read.
        sendEvent(m_events->forIStream().inputShutdown());
        if (!m_writable && m_inputBuffer.getSize() == 0) {
            sendEvent(m_events->forISocket().disconnected());
            m_connected = false;
        }
        m_readable = false;
        return kNew;
    }

    return kRetry;
}

TCPSocket::EJobResult
SecureSocket::doWrite()
{
    // write data
    int bufferSize = 0;
    int bytesWrote = 0;
    int status = 0;

    if (!isSecureReady())
        return kRetry;

    if (do_write_retry_) {
        bufferSize = do_write_retry_size_;
    } else {
        bufferSize = m_outputBuffer.getSize();
        if (bufferSize > do_write_retry_buffer_size_) {
            do_write_retry_buffer_.reset(new char[bufferSize]);
            do_write_retry_buffer_size_ = bufferSize;
        }
        if (bufferSize > 0) {
            std::memcpy(do_write_retry_buffer_.get(), m_outputBuffer.peek(bufferSize), bufferSize);
        }
    }

    if (bufferSize == 0) {
        return kRetry;
    }

    status = secureWrite(do_write_retry_buffer_.get(), bufferSize, bytesWrote);
    if (status > 0) {
        do_write_retry_ = false;
    } else if (status < 0) {
        return kBreak;
    } else if (status == 0) {
        do_write_retry_ = true;
        do_write_retry_size_ = bufferSize;
        return kNew;
    }

    if (bytesWrote > 0) {
        discardWrittenData(bytesWrote);
        return kNew;
    }

    return kRetry;
}

int
SecureSocket::secureRead(void* buffer, int size, int& read)
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    if (m_ssl->m_ssl != NULL) {
        LOG((CLOG_DEBUG2 "reading secure socket"));
        read = SSL_read(m_ssl->m_ssl, buffer, size);

        // Check result will cleanup the connection in the case of a fatal
        checkResult(read, secure_read_retry_);

        if (secure_read_retry_) {
            return 0;
        }

        if (isFatal()) {
            return -1;
        }
    }
    // According to SSL spec, the number of bytes read must not be negative and
    // not have an error code from SSL_get_error(). If this happens, it is
    // itself an error. Let the parent handle the case
    return read;
}

int
SecureSocket::secureWrite(const void* buffer, int size, int& wrote)
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    if (m_ssl->m_ssl != NULL) {
        LOG((CLOG_DEBUG2 "writing secure socket:%p", this));

        wrote = SSL_write(m_ssl->m_ssl, buffer, size);

        // Check result will cleanup the connection in the case of a fatal
        checkResult(wrote, secure_write_retry_);

        if (secure_write_retry_) {
            return 0;
        }

        if (isFatal()) {
            return -1;
        }
    }
    // According to SSL spec, r must not be negative and not have an error code
    // from SSL_get_error(). If this happens, it is itself an error. Let the
    // parent handle the case
    return wrote;
}

bool
SecureSocket::isSecureReady()
{
    return m_secureReady;
}

void
SecureSocket::initSsl(bool server)
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    m_ssl = new Ssl();
    m_ssl->m_context = NULL;
    m_ssl->m_ssl = NULL;

    initContext(server);
}

bool SecureSocket::load_certificates(const barrier::fs::path& path)
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    if (path.empty()) {
        showError("ssl certificate is not specified");
        return false;
    }
    else {
        if (!barrier::fs::is_regular_file(path)) {
            showError("ssl certificate doesn't exist: " + path.u8string());
            return false;
        }
    }

    int r = 0;
    r = SSL_CTX_use_certificate_file(m_ssl->m_context, path.u8string().c_str(), SSL_FILETYPE_PEM);
    if (r <= 0) {
        showError("could not use ssl certificate: " + path.u8string());
        return false;
    }

    r = SSL_CTX_use_PrivateKey_file(m_ssl->m_context, path.u8string().c_str(), SSL_FILETYPE_PEM);
    if (r <= 0) {
        showError("could not use ssl private key: " + path.u8string());
        return false;
    }

    r = SSL_CTX_check_private_key(m_ssl->m_context);
    if (!r) {
        showError("could not verify ssl private key: " + path.u8string());
        return false;
    }

    return true;
}

static int cert_verify_ignore_callback(X509_STORE_CTX*, void*)
{
    return 1;
}

void
SecureSocket::initContext(bool server)
{
    // ssl_mutex_ is assumed to be acquired

    SSL_library_init();

    const SSL_METHOD* method;

    // load & register all cryptos, etc.
    OpenSSL_add_all_algorithms();

    // load all error messages
    SSL_load_error_strings();

    if (CLOG->getFilter() >= kINFO) {
        showSecureLibInfo();
    }

    // SSLv23_method uses TLSv1, with the ability to fall back to SSLv3
    if (server) {
        method = SSLv23_server_method();
    }
    else {
        method = SSLv23_client_method();
    }

    // create new context from method
    SSL_METHOD* m = const_cast<SSL_METHOD*>(method);
    m_ssl->m_context = SSL_CTX_new(m);

    // drop SSLv3 support
    SSL_CTX_set_options(m_ssl->m_context, SSL_OP_NO_SSLv3);

    if (m_ssl->m_context == NULL) {
        showError("");
    }

    if (security_level_ == ConnectionSecurityLevel::ENCRYPTED_AUTHENTICATED) {
        // We want to ask for peer certificate, but not verify it. If we don't ask for peer
        // certificate, e.g. client won't send it.
        SSL_CTX_set_verify(m_ssl->m_context, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
                           nullptr);
        SSL_CTX_set_cert_verify_callback(m_ssl->m_context, cert_verify_ignore_callback, nullptr);
    }
}

void
SecureSocket::createSSL()
{
    // ssl_mutex_ is assumed to be acquired

    // I assume just one instance is needed
    // get new SSL state with context
    if (m_ssl->m_ssl == NULL) {
        assert(m_ssl->m_context != NULL);
        m_ssl->m_ssl = SSL_new(m_ssl->m_context);
    }
}

int
SecureSocket::secureAccept(int socket)
{
    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    createSSL();

    // set connection socket to SSL state
    SSL_set_fd(m_ssl->m_ssl, socket);

    LOG((CLOG_DEBUG2 "accepting secure socket"));
    int r = SSL_accept(m_ssl->m_ssl);

    checkResult(r, secure_accept_retry_);

    if (isFatal()) {
        // tell user and sleep so the socket isn't hammered.
        LOG((CLOG_ERR "failed to accept secure socket"));
        LOG((CLOG_INFO "client connection may not be secure"));
        m_secureReady = false;
        ARCH->sleep(1);
        secure_accept_retry_ = 0;
        return -1; // Failed, error out
    }

    // If not fatal and no retry, state is good
    if (secure_accept_retry_ == 0) {
        if (security_level_ == ConnectionSecurityLevel::ENCRYPTED_AUTHENTICATED) {
            if (verify_cert_fingerprint(
                        barrier::DataDirectories::trusted_clients_ssl_fingerprints_path())) {
                LOG((CLOG_INFO "accepted secure socket"));
                if (!ensure_peer_certificate()) {
                    secure_accept_retry_ = 0;
                    disconnect();
                    return -1;// Cert fail, error
                }
            }
            else {
                LOG((CLOG_ERR "failed to verify server certificate fingerprint"));
                secure_accept_retry_ = 0;
                disconnect();
                return -1; // Fingerprint failed, error
            }
        }

        m_secureReady = true;
        LOG((CLOG_INFO "accepted secure socket"));
        if (CLOG->getFilter() >= kDEBUG1) {
            showSecureCipherInfo();
        }
        showSecureConnectInfo();
        return 1;
    }

    // If not fatal and retry is set, not ready, and return retry
    if (secure_accept_retry_ > 0) {
        LOG((CLOG_DEBUG2 "retry accepting secure socket"));
        m_secureReady = false;
        ARCH->sleep(s_retryDelay);
        return 0;
    }

    // no good state exists here
    LOG((CLOG_ERR "unexpected state attempting to accept connection"));
    return -1;
}

int
SecureSocket::secureConnect(int socket)
{
    // note that load_certificates acquires ssl_mutex_
    if (!load_certificates(barrier::DataDirectories::ssl_certificate_path())) {
        LOG((CLOG_ERR "could not load client certificates"));
        // FIXME: this is fatal error, but we current don't disconnect because whole logic in this
        // function needs to be cleaned up
    }

    std::lock_guard<std::mutex> ssl_lock{ssl_mutex_};

    createSSL();

    // attach the socket descriptor
    SSL_set_fd(m_ssl->m_ssl, socket);

    LOG((CLOG_DEBUG2 "connecting secure socket"));
    int r = SSL_connect(m_ssl->m_ssl);

    checkResult(r, secure_connect_retry_);

    if (isFatal()) {
        LOG((CLOG_ERR "failed to connect secure socket"));
        secure_connect_retry_ = 0;
        return -1;
    }

    // If we should retry, not ready and return 0
    if (secure_connect_retry_ > 0) {
        LOG((CLOG_DEBUG2 "retry connect secure socket"));
        m_secureReady = false;
        ARCH->sleep(s_retryDelay);
        return 0;
    }

    secure_connect_retry_ = 0;
    // No error, set ready, process and return ok
    m_secureReady = true;
    if (verify_cert_fingerprint(barrier::DataDirectories::trusted_servers_ssl_fingerprints_path())) {
        LOG((CLOG_INFO "connected to secure socket"));
        if (!ensure_peer_certificate()) {
            disconnect();
            return -1;// Cert fail, error
        }
    }
    else {
        LOG((CLOG_ERR "failed to verify server certificate fingerprint"));
        disconnect();
        return -1; // Fingerprint failed, error
    }
    LOG((CLOG_DEBUG2 "connected secure socket"));
    if (CLOG->getFilter() >= kDEBUG1) {
        showSecureCipherInfo();
    }
    showSecureConnectInfo();
    return 1;
}

bool
SecureSocket::ensure_peer_certificate()
{
    // ssl_mutex_ is assumed to be acquired
    X509* cert;
    char* line;

    // get the server's certificate
    cert = SSL_get_peer_certificate(m_ssl->m_ssl);
    if (cert != NULL) {
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        LOG((CLOG_INFO "peer ssl certificate info: %s", line));
        OPENSSL_free(line);
        X509_free(cert);
    }
    else {
        showError("peer has no ssl certificate");
        return false;
    }

    return true;
}

void
SecureSocket::checkResult(int status, int& retry)
{
    // ssl_mutex_ is assumed to be acquired

    // ssl errors are a little quirky. the "want" errors are normal and
    // should result in a retry.

    int errorCode = SSL_get_error(m_ssl->m_ssl, status);

    switch (errorCode) {
    case SSL_ERROR_NONE:
        retry = 0;
        // operation completed
        break;

    case SSL_ERROR_ZERO_RETURN:
        // connection closed
        isFatal(true);
        LOG((CLOG_DEBUG "ssl connection closed"));
        break;

    case SSL_ERROR_WANT_READ:
        retry++;
        LOG((CLOG_DEBUG2 "want to read, error=%d, attempt=%d", errorCode, retry));
        break;

    case SSL_ERROR_WANT_WRITE:
        // Need to make sure the socket is known to be writable so the impending
        // select action actually triggers on a write. This isn't necessary for
        // m_readable because the socket logic is always readable
        m_writable = true;
        retry++;
        LOG((CLOG_DEBUG2 "want to write, error=%d, attempt=%d", errorCode, retry));
        break;

    case SSL_ERROR_WANT_CONNECT:
        retry++;
        LOG((CLOG_DEBUG2 "want to connect, error=%d, attempt=%d", errorCode, retry));
        break;

    case SSL_ERROR_WANT_ACCEPT:
        retry++;
        LOG((CLOG_DEBUG2 "want to accept, error=%d, attempt=%d", errorCode, retry));
        break;

    case SSL_ERROR_SYSCALL:
        LOG((CLOG_ERR "ssl error occurred (system call failure)"));
        if (ERR_peek_error() == 0) {
            if (status == 0) {
                LOG((CLOG_ERR "eof violates ssl protocol"));
            }
            else if (status == -1) {
                // underlying socket I/O reproted an error
                try {
                    ARCH->throwErrorOnSocket(getSocket());
                }
                catch (XArchNetwork& e) {
                    LOG((CLOG_ERR "%s", e.what()));
                }
            }
        }

        isFatal(true);
        break;

    case SSL_ERROR_SSL:
        LOG((CLOG_ERR "ssl error occurred (generic failure)"));
        isFatal(true);
        break;

    default:
        LOG((CLOG_ERR "ssl error occurred (unknown failure)"));
        isFatal(true);
        break;
    }

    if (isFatal()) {
        retry = 0;
        showError("");
        disconnect();
    }
}

void SecureSocket::showError(const std::string& reason)
{
    if (!reason.empty()) {
        LOG((CLOG_ERR "%s", reason.c_str()));
    }

    std::string error = getError();
    if (!error.empty()) {
        LOG((CLOG_ERR "%s", error.c_str()));
    }
}

std::string SecureSocket::getError()
{
    unsigned long e = ERR_get_error();

    if (e != 0) {
        char error[MAX_ERROR_SIZE];
        ERR_error_string_n(e, error, MAX_ERROR_SIZE);
        return error;
    }
    else {
        return "";
    }
}

void
SecureSocket::disconnect()
{
    sendEvent(getEvents()->forISocket().stopRetry());
    sendEvent(getEvents()->forISocket().disconnected());
    sendEvent(getEvents()->forIStream().inputShutdown());
}

bool SecureSocket::verify_cert_fingerprint(const barrier::fs::path& fingerprint_db_path)
{
    // ssl_mutex_ is assumed to be acquired

    // calculate received certificate fingerprint
    barrier::FingerprintData fingerprint_sha1, fingerprint_sha256;
    try {
        auto* cert = SSL_get_peer_certificate(m_ssl->m_ssl);
        fingerprint_sha1 = barrier::get_ssl_cert_fingerprint(cert,
                                                             barrier::FingerprintType::SHA1);
        fingerprint_sha256 = barrier::get_ssl_cert_fingerprint(cert,
                                                               barrier::FingerprintType::SHA256);
    } catch (const std::exception& e) {
        LOG((CLOG_ERR "%s", e.what()));
        return false;
    }

    // note: the GUI parses the following two lines of logs, don't change unnecessarily
    LOG((CLOG_NOTE "peer fingerprint (SHA1): %s (SHA256): %s",
         barrier::format_ssl_fingerprint(fingerprint_sha1.data).c_str(),
         barrier::format_ssl_fingerprint(fingerprint_sha256.data).c_str()));

    // Provide debug hint as to what file is being used to verify fingerprint trust
    LOG((CLOG_NOTE "fingerprint_db_path: %s", fingerprint_db_path.u8string().c_str()));

    barrier::FingerprintDatabase db;
    db.read(fingerprint_db_path);

    if (!db.fingerprints().empty()) {
        LOG((CLOG_NOTE "Read %d fingerprints from: %s", db.fingerprints().size(),
             fingerprint_db_path.u8string().c_str()));
    } else {
        LOG((CLOG_NOTE "Could not read fingerprints from: %s",
             fingerprint_db_path.u8string().c_str()));
    }

    if (db.is_trusted(fingerprint_sha256)) {
        LOG((CLOG_NOTE "Fingerprint matches trusted fingerprint"));
        return true;
    } else {
        LOG((CLOG_NOTE "Fingerprint does not match trusted fingerprint"));
        return false;
    }
}

MultiplexerJobStatus SecureSocket::serviceConnect(ISocketMultiplexerJob* job,
                                                  bool read, bool write, bool error)
{
    (void) read;

    Lock lock(&getMutex());

    int status = 0;
#ifdef SYSAPI_WIN32
    status = secureConnect(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
    status = secureConnect(getSocket()->m_fd);
#endif

    // If status < 0, error happened
    if (status < 0) {
        return {false, {}};
    }

    // If status > 0, success
    if (status > 0) {
        sendEvent(m_events->forIDataSocket().secureConnected());
        return newJobOrStopServicing();
    }

    // Retry case
    return {
        true,
        std::make_unique<TSocketMultiplexerMethodJob>([this](auto j, auto r, auto w, auto e)
                                                      { return serviceConnect(j, r, w, e); },
                                                      getSocket(), isReadable(), isWritable())
    };
}

MultiplexerJobStatus SecureSocket::serviceAccept(ISocketMultiplexerJob* job,
                                                 bool read, bool write, bool error)
{
    (void) read;
    Lock lock(&getMutex());

    int status = 0;
#ifdef SYSAPI_WIN32
    status = secureAccept(static_cast<int>(getSocket()->m_socket));
#elif SYSAPI_UNIX
    status = secureAccept(getSocket()->m_fd);
#endif
        // If status < 0, error happened
    if (status < 0) {
        return {false, {}};
    }

    // If status > 0, success
    if (status > 0) {
        sendEvent(m_events->forClientListener().accepted());
        return newJobOrStopServicing();
    }

    // Retry case
    return {
        true,
        std::make_unique<TSocketMultiplexerMethodJob>([this](auto j, auto r, auto w, auto e)
                                                      { return serviceAccept(j, r, w, e); },
                                                      getSocket(), isReadable(), isWritable())
    };
}

void
showCipherStackDesc(STACK_OF(SSL_CIPHER) * stack) {
    char msg[kMsgSize];
    int i = 0;
    for ( ; i < sk_SSL_CIPHER_num(stack) ; i++) {
        const SSL_CIPHER * cipher = sk_SSL_CIPHER_value(stack,i);

        SSL_CIPHER_description(cipher, msg, kMsgSize);

        // Why does SSL put a newline in the description?
        int pos = (int)strlen(msg) - 1;
        if (msg[pos] == '\n') {
            msg[pos] = '\0';
        }

        LOG((CLOG_DEBUG1 "%s",msg));
    }
}

void
SecureSocket::showSecureCipherInfo()
{
    // ssl_mutex_ is assumed to be acquired

    STACK_OF(SSL_CIPHER) * sStack = SSL_get_ciphers(m_ssl->m_ssl);

    if (sStack == NULL) {
        LOG((CLOG_DEBUG1 "local cipher list not available"));
    }
    else {
        LOG((CLOG_DEBUG1 "available local ciphers:"));
        showCipherStackDesc(sStack);
    }

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	// m_ssl->m_ssl->session->ciphers is not forward compatible,
	// In future release of OpenSSL, it's not visible,
    STACK_OF(SSL_CIPHER) * cStack = m_ssl->m_ssl->session->ciphers;
#else
	// Use SSL_get_client_ciphers() for newer versions
	STACK_OF(SSL_CIPHER) * cStack = SSL_get_client_ciphers(m_ssl->m_ssl);
#endif
	if (cStack == NULL) {
        LOG((CLOG_DEBUG1 "remote cipher list not available"));
    }
    else {
        LOG((CLOG_DEBUG1 "available remote ciphers:"));
        showCipherStackDesc(cStack);
    }
    return;
}

void
SecureSocket::showSecureLibInfo()
{
    LOG((CLOG_INFO "%s",SSLeay_version(SSLEAY_VERSION)));
    LOG((CLOG_DEBUG1 "openSSL : %s",SSLeay_version(SSLEAY_CFLAGS)));
    LOG((CLOG_DEBUG1 "openSSL : %s",SSLeay_version(SSLEAY_BUILT_ON)));
    LOG((CLOG_DEBUG1 "openSSL : %s",SSLeay_version(SSLEAY_PLATFORM)));
    LOG((CLOG_DEBUG1 "%s",SSLeay_version(SSLEAY_DIR)));
    return;
}

void
SecureSocket::showSecureConnectInfo()
{
    // ssl_mutex_ is assumed to be acquired

    const SSL_CIPHER* cipher = SSL_get_current_cipher(m_ssl->m_ssl);

    if (cipher != NULL) {
        char msg[kMsgSize];
        SSL_CIPHER_description(cipher, msg, kMsgSize);
        LOG((CLOG_INFO "%s", msg));
        }
    return;
}

void
SecureSocket::handleTCPConnected(const Event& event, void*)
{
    if (getSocket() == nullptr) {
        LOG((CLOG_DEBUG "disregarding stale connect event"));
        return;
    }
    secureConnect();
}
