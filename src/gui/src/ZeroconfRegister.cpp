/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "ZeroconfRegister.h"

#include <QtCore/QSocketNotifier>

ZeroconfRegister::ZeroconfRegister(QObject* parent) :
    QObject(parent),
    m_DnsServiceRef(nullptr)
{
}

ZeroconfRegister::~ZeroconfRegister()
{
    if (m_DnsServiceRef) {
        DNSServiceRefDeallocate(m_DnsServiceRef);
        m_DnsServiceRef = nullptr;
    }
}

void ZeroconfRegister::registerService(const ZeroconfRecord& record,
    quint16 servicePort)
{
    if (m_DnsServiceRef) {
        qWarning("Warning: Already registered a service for this object");
        return;
    }

    quint16 bigEndianPort = servicePort;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    {
        bigEndianPort =  0 | ((servicePort & 0x00ff) << 8) | ((servicePort & 0xff00) >> 8);
    }
#endif

    DNSServiceErrorType err = DNSServiceRegister(&m_DnsServiceRef,
        kDNSServiceFlagsNoAutoRename, 0,
        record.serviceName.toUtf8().constData(),
        record.registeredType.toUtf8().constData(),
        record.replyDomain.isEmpty() ? nullptr : record.replyDomain.toUtf8().constData(),
        nullptr, bigEndianPort, 0, nullptr, registerService, this);

    if (err != kDNSServiceErr_NoError) {
        Q_EMIT error(err);
    }
    else {
        int sockfd = DNSServiceRefSockFD(m_DnsServiceRef);
        if (sockfd == -1) {
            Q_EMIT error(kDNSServiceErr_Invalid);
        }
        else {
            socket_ = std::make_unique<QSocketNotifier>(sockfd, QSocketNotifier::Read, this);
            connect(socket_.get(), &QSocketNotifier::activated, this, &ZeroconfRegister::socketReadyRead);
        }
    }
}

void ZeroconfRegister::socketReadyRead()
{
    DNSServiceErrorType err = DNSServiceProcessResult(m_DnsServiceRef);
    if (err != kDNSServiceErr_NoError) {
        Q_EMIT error(err);
    }
}

void ZeroconfRegister::registerService(DNSServiceRef, DNSServiceFlags,
        DNSServiceErrorType errorCode, const char* name, const char* regtype,
        const char* domain, void* data)
{
    (void) name;
    (void) regtype;
    (void) domain;

    ZeroconfRegister* serviceRegister = static_cast<ZeroconfRegister*>(data);
    if (errorCode != kDNSServiceErr_NoError) {
        Q_EMIT serviceRegister->error(errorCode);
    }
}
