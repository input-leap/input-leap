/*
    barrier -- mouse and keyboard sharing utility
    Copyright (C) Barrier contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "SecureUtils.h"
#include "base/String.h"
#include "base/finally.h"
#include "io/fstream.h"

#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <cstdio>
#include <stdexcept>

namespace barrier {

namespace {

const EVP_MD* get_digest_for_type(FingerprintType type)
{
    switch (type) {
        case FingerprintType::SHA1: return EVP_sha1();
        case FingerprintType::SHA256: return EVP_sha256();
    }
    throw std::runtime_error("Unknown fingerprint type " + std::to_string(static_cast<int>(type)));
}

} // namespace

std::string format_ssl_fingerprint(const std::vector<uint8_t>& fingerprint, bool separator)
{
    std::string result = barrier::string::to_hex(fingerprint, 2);

    // all uppercase
    barrier::string::uppercase(result);

    if (separator) {
        // add colon to separate each 2 characters
        size_t separators = result.size() / 2;
        for (size_t i = 1; i < separators; i++) {
            result.insert(i * 3 - 1, ":");
        }
    }
    return result;
}

std::vector<std::uint8_t> get_ssl_cert_fingerprint(X509* cert, FingerprintType type)
{
    if (!cert) {
        throw std::runtime_error("certificate is null");
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_length = 0;
    int result = X509_digest(cert, get_digest_for_type(type), digest, &digest_length);

    if (result <= 0) {
        throw std::runtime_error("failed to calculate fingerprint, digest result: " +
                                 std::to_string(result));
    }

    std::vector<std::uint8_t> digest_vec;
    digest_vec.assign(reinterpret_cast<std::uint8_t*>(digest),
                      reinterpret_cast<std::uint8_t*>(digest) + digest_length);
    return digest_vec;
}

std::vector<std::uint8_t> get_pem_file_cert_fingerprint(const std::string& path,
                                                        FingerprintType type)
{
    auto fp = fopen_utf8_path(path, "r");
    if (!fp) {
        throw std::runtime_error("Could not open certificate path");
    }
    auto file_close = finally([fp]() { std::fclose(fp); });

    X509* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert) {
        throw std::runtime_error("Certificate could not be parsed");
    }
    auto cert_free = finally([cert]() { X509_free(cert); });

    return get_ssl_cert_fingerprint(cert, type);
}

void generate_pem_self_signed_cert(const std::string& path)
{
    auto expiration_days = 365;

    auto* private_key = EVP_PKEY_new();
    if (!private_key) {
        throw std::runtime_error("Could not allocate private key for certificate");
    }
    auto private_key_free = finally([private_key](){ EVP_PKEY_free(private_key); });

    auto* rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    if (!rsa) {
        throw std::runtime_error("Failed to generate RSA key");
    }
    EVP_PKEY_assign_RSA(private_key, rsa);

    auto* cert = X509_new();
    if (!cert) {
        throw std::runtime_error("Could not allocate certificate");
    }
    auto cert_free = finally([cert]() { X509_free(cert); });

    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), expiration_days * 24 * 3600);
    X509_set_pubkey(cert, private_key);

    auto* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                               reinterpret_cast<const unsigned char *>("Barrier"), -1, -1, 0);
    X509_set_issuer_name(cert, name);

    X509_sign(cert, private_key, EVP_sha256());

    auto fp = fopen_utf8_path(path.c_str(), "r");
    if (!fp) {
        throw std::runtime_error("Could not open certificate output path");
    }
    auto file_close = finally([fp]() { std::fclose(fp); });

    PEM_write_PrivateKey(fp, private_key, nullptr, nullptr, 0, nullptr, nullptr);
    PEM_write_X509(fp, cert);
}

} // namespace barrier
