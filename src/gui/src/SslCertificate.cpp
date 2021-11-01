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

#include "SslCertificate.h"
#include "common/DataDirectories.h"
#include "base/finally.h"
#include "io/fstream.h"
#include "net/FingerprintDatabase.h"
#include "net/SecureUtils.h"

#include <QProcess>
#include <QDir>
#include <QCoreApplication>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

static const char kCertificateFilename[] = "Barrier.pem";
static const char kSslDir[] = "SSL";

SslCertificate::SslCertificate(QObject *parent) :
    QObject(parent)
{
    m_ProfileDir = DataDirectories::profile();
    if (m_ProfileDir.empty()) {
        emit error(tr("Failed to get profile directory."));
    }
}

void SslCertificate::generateCertificate()
{
    auto cert_path = getCertificatePath();

    QFile file(QString::fromStdString(cert_path));
    if (!file.exists() || !isCertificateValid(cert_path)) {
        QDir sslDir(QString::fromStdString(getCertificateDirectory()));
        if (!sslDir.exists()) {
            sslDir.mkpath(".");
        }

        try {
            barrier::generate_pem_self_signed_cert(cert_path);
        }  catch (const std::exception& e) {
            emit error(QString("SSL tool failed: %1").arg(e.what()));
            return;
        }

        emit info(tr("SSL certificate generated."));
    }

    generateFingerprint(cert_path);

    emit generateFinished();
}

void SslCertificate::generateFingerprint(const std::string& cert_path)
{
    try {
        auto local_path = DataDirectories::local_ssl_fingerprints_path();
        barrier::FingerprintDatabase db;
        db.add_trusted(barrier::get_pem_file_cert_fingerprint(cert_path,
                                                              barrier::FingerprintType::SHA1));
        db.add_trusted(barrier::get_pem_file_cert_fingerprint(cert_path,
                                                              barrier::FingerprintType::SHA256));
        db.write(local_path);

        emit info(tr("SSL fingerprint generated."));
    } catch (const std::exception& e) {
        emit error(tr("Failed to find SSL fingerprint.") + e.what());
    }
}

std::string SslCertificate::getCertificatePath()
{
    return getCertificateDirectory() + QDir::separator().toLatin1() + kCertificateFilename;
}

std::string SslCertificate::getCertificateDirectory()
{
    return m_ProfileDir + QDir::separator().toLatin1() + kSslDir;
}

bool SslCertificate::isCertificateValid(const std::string& path)
{
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();

    auto fp = barrier::fopen_utf8_path(path, "r");
    if (!fp) {
        emit info(tr("Could not read from default certificate file."));
        return false;
    }
    auto file_close = barrier::finally([fp]() { std::fclose(fp); });

    auto* cert = PEM_read_X509(fp, nullptr, nullptr, nullptr);
    if (!cert) {
        emit info(tr("Error loading default certificate file to memory."));
        return false;
    }
    auto cert_free = barrier::finally([cert]() { X509_free(cert); });

    auto* pubkey = X509_get_pubkey(cert);
    if (!pubkey) {
        emit info(tr("Default certificate key file does not contain valid public key"));
        return false;
    }
    auto pubkey_free = barrier::finally([pubkey]() { EVP_PKEY_free(pubkey); });

    auto type = EVP_PKEY_type(EVP_PKEY_id(pubkey));
    if (type != EVP_PKEY_RSA && type != EVP_PKEY_DSA) {
        emit info(tr("Public key in default certificate key file is not RSA or DSA"));
        return false;
    }

    auto bits = EVP_PKEY_bits(pubkey);
    if (bits < 2048) {
        // We could have small keys in old barrier installations
        emit info(tr("Public key in default certificate key file is too small."));
        return false;
    }

    return true;
}
