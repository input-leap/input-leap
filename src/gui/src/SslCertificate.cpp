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

#include "SslCertificate.h"
#include "common/DataDirectories.h"
#include "base/finally.h"
#include "io/filesystem.h"
#include "net/FingerprintDatabase.h"
#include "net/SecureUtils.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

SslCertificate::SslCertificate(QObject *parent) :
    QObject(parent)
{
    if (inputleap::DataDirectories::profile().empty()) {
        emit error(tr("Failed to get profile directory."));
    }
}

void SslCertificate::generateCertificate()
{
    auto cert_path = inputleap::DataDirectories::ssl_certificate_path();

    if (!inputleap::fs::exists(cert_path) || !is_certificate_valid(cert_path)) {
        try {
            auto cert_dir = cert_path.parent_path();
            if (!inputleap::fs::exists(cert_dir)) {
                inputleap::fs::create_directories(cert_dir);
            }

            inputleap::generate_pem_self_signed_cert(cert_path.u8string());
        }  catch (const std::exception& e) {
            emit error(QString("SSL tool failed: %1").arg(e.what()));
            return;
        }

        emit info(tr("SSL certificate generated."));
    }

    generate_fingerprint(cert_path);

    emit generateFinished();
}

void SslCertificate::generate_fingerprint(const inputleap::fs::path& cert_path)
{
    try {
        auto local_path = inputleap::DataDirectories::local_ssl_fingerprints_path();
        auto local_dir = local_path.parent_path();
        if (!inputleap::fs::exists(local_dir)) {
            inputleap::fs::create_directories(local_dir);
        }

        inputleap::FingerprintDatabase db;
        db.add_trusted(inputleap::get_pem_file_cert_fingerprint(cert_path.u8string(),
                                                              inputleap::FingerprintType::SHA1));
        db.add_trusted(inputleap::get_pem_file_cert_fingerprint(cert_path.u8string(),
                                                              inputleap::FingerprintType::SHA256));
        db.write(local_path);

        emit info(tr("SSL fingerprint generated."));
    } catch (const std::exception& e) {
        emit error(tr("Failed to find SSL fingerprint.") + e.what());
    }
}

bool SslCertificate::is_certificate_valid(const inputleap::fs::path& path)
{
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    auto fp = inputleap::fopen_utf8_path(path, "r");
    if (!fp) {
        emit info(tr("Could not read from default certificate file."));
        return false;
    }
    auto file_close = inputleap::finally([fp]() { std::fclose(fp); });

    auto* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert) {
        emit info(tr("Error loading default certificate file to memory."));
        return false;
    }
    auto cert_free = inputleap::finally([cert]() { X509_free(cert); });

    auto* pubkey = X509_get_pubkey(cert);
    if (!pubkey) {
        emit info(tr("Default certificate key file does not contain valid public key"));
        return false;
    }
    auto pubkey_free = inputleap::finally([pubkey]() { EVP_PKEY_free(pubkey); });

    auto type = EVP_PKEY_type(EVP_PKEY_id(pubkey));
    if (type != EVP_PKEY_RSA && type != EVP_PKEY_DSA) {
        emit info(tr("Public key in default certificate key file is not RSA or DSA"));
        return false;
    }

    auto bits = EVP_PKEY_bits(pubkey);
    if (bits < 2048) {
        // We could have small keys in old InputLeap installations
        emit info(tr("Public key in default certificate key file is too small."));
        return false;
    }

    return true;
}
