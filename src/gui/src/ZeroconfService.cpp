/*
 * barrier -- mouse and keyboard sharing utility
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

#include "ZeroconfService.h"

#include "MainWindow.h"
#include "ZeroconfRegister.h"
#include "ZeroconfBrowser.h"

#include <QtNetwork>
#include <QMessageBox>
#define _MSL_STDINT_H
#include <stdint.h>
#include <dns_sd.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wlanapi.h>
#pragma comment(lib, "wlanapi.lib")
#else
#include <stdlib.h>
#endif

const char* ZeroconfService:: m_ServerServiceName = "_barrierServerZeroconf._tcp";
const char* ZeroconfService:: m_ClientServiceName = "_barrierClientZeroconf._tcp";

static void silence_avahi_warning()
{
    // the libavahi folks seemingly find Apple's bonjour API distasteful
    // and are quite liberal in taking it out on users...unless we set
    // this environmental variable before calling the avahi library.
    // additionally, Microsoft does not give us a POSIX setenv() so
    // we use their OS-specific API instead
    const char *name  = "AVAHI_COMPAT_NOWARN";
    const char *value = "1";
#ifdef _WIN32
    SetEnvironmentVariable(name, value);
#else
    setenv(name, value, 1);
#endif
}

ZeroconfService::ZeroconfService(MainWindow* mainWindow) :
    m_pMainWindow(mainWindow),
    m_pZeroconfBrowser(0),
    m_pZeroconfRegister(0),
    m_ServiceRegistered(false)
{
    silence_avahi_warning();
    if (m_pMainWindow->barrierType() == MainWindow::barrierServer) {
        if (registerService(true)) {
            m_pZeroconfBrowser = new ZeroconfBrowser(this);
            connect(m_pZeroconfBrowser, SIGNAL(
                currentRecordsChanged(const QList<ZeroconfRecord>&)),
                this, SLOT(clientDetected(const QList<ZeroconfRecord>&)));
            m_pZeroconfBrowser->browseForType(
                QLatin1String(m_ClientServiceName));
        }
    }
    else {
        m_pZeroconfBrowser = new ZeroconfBrowser(this);
        connect(m_pZeroconfBrowser, SIGNAL(
            currentRecordsChanged(const QList<ZeroconfRecord>&)),
            this, SLOT(serverDetected(const QList<ZeroconfRecord>&)));
        m_pZeroconfBrowser->browseForType(
            QLatin1String(m_ServerServiceName));
    }

    connect(m_pZeroconfBrowser, SIGNAL(error(DNSServiceErrorType)),
        this, SLOT(errorHandle(DNSServiceErrorType)));
}

ZeroconfService::~ZeroconfService()
{
    if (m_pZeroconfBrowser) {
        delete m_pZeroconfBrowser;
    }
    if (m_pZeroconfRegister) {
        delete m_pZeroconfRegister;
    }
}

void ZeroconfService::serverDetected(const QList<ZeroconfRecord>& list)
{
    foreach (ZeroconfRecord record, list) {
        registerService(false);
        m_pMainWindow->appendLogInfo(tr("zeroconf server detected: %1").arg(
            record.serviceName));
        m_pMainWindow->serverDetected(record.serviceName);
    }
}

void ZeroconfService::clientDetected(const QList<ZeroconfRecord>& list)
{
    foreach (ZeroconfRecord record, list) {
        m_pMainWindow->appendLogInfo(tr("zeroconf client detected: %1").arg(
            record.serviceName));
        m_pMainWindow->autoAddScreen(record.serviceName);
    }
}

void ZeroconfService::errorHandle(DNSServiceErrorType errorCode)
{
    QMessageBox::critical(0, tr("Zero configuration service"),
        tr("Error code: %1.").arg(errorCode));
}

#ifdef _WIN32
static QString mac_to_string(DOT11_MAC_ADDRESS mac)
{
    char str[18];
    snprintf(str, 18, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return str;
}

static QList<QString> wireless_mac_strings()
{
    QList<QString> wlanMAC;
    DWORD version;
    HANDLE client;
    if (WlanOpenHandle(2, NULL, &version, &client) == 0) {
        WLAN_INTERFACE_INFO_LIST * list;
        if (WlanEnumInterfaces(client, NULL, &list) == 0) {
            for (DWORD idx = 0; idx < list->dwNumberOfItems; ++idx) {
                const GUID * PGuid = &(list->InterfaceInfo[0].InterfaceGuid);
                const WLAN_INTF_OPCODE OpCode = wlan_intf_opcode_current_connection;
                WLAN_CONNECTION_ATTRIBUTES * attrs;
                DWORD attrsSz;
                if (WlanQueryInterface(client, PGuid, OpCode, NULL, &attrsSz,
                    (void**)&attrs, NULL) == 0) {
                    wlanMAC.append(mac_to_string(
                        attrs->wlanAssociationAttributes.dot11Bssid));
                    WlanFreeMemory(attrs);
                }
            }
            WlanFreeMemory(list);
        }
        WlanCloseHandle(client, NULL);
    }
    return wlanMAC;
}
#else
static QList<QString> wireless_mac_strings()
{
    // TODO
    return QList<QString>();
}
#endif

QString ZeroconfService::getLocalIPAddresses()
{
    const QString NonEthernetMAC = "00:00:00:00:00:00";
    const auto wlanMAC = wireless_mac_strings();
    QString wirelessIP = "";
    foreach(const auto qni, QNetworkInterface::allInterfaces()) {
        // weed out loopback, inactive, and non-ethernet interfaces
        if (!qni.flags().testFlag(QNetworkInterface::IsLoopBack) &&
            qni.flags().testFlag(QNetworkInterface::IsUp) &&
            qni.hardwareAddress() != NonEthernetMAC) {
            bool isWireless = wlanMAC.contains(qni.hardwareAddress().toUpper());
            foreach(const auto address, qni.allAddresses()) {
                if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                    // use the first non-wireless address we find
                    if (!isWireless)
                        return address.toString();
                    // save the first wireless address we find
                    if (wirelessIP.isEmpty())
                        wirelessIP = address.toString();
                }
            }
        }
    }
    // if no non-wireless address could be found use a wireless one
    // if one was found. otherwise return an empty string
    return wirelessIP;
}

bool ZeroconfService::registerService(bool server)
{
    bool result = true;

    if (!m_ServiceRegistered) {
        if (!m_zeroconfServer.listen()) {
            QMessageBox::critical(0, tr("Zero configuration service"),
                tr("Unable to start the zeroconf: %1.")
                .arg(m_zeroconfServer.errorString()));
            result = false;
        }
        else {
            m_pZeroconfRegister = new ZeroconfRegister(this);
            if (server) {
                QString localIP = getLocalIPAddresses();
                if (localIP.isEmpty()) {
                    QMessageBox::warning(m_pMainWindow, tr("Barrier"),
                        tr("Failed to get local IP address. "
                           "Please manually type in server address "
                           "on your clients"));
                }
                else {
                    m_pZeroconfRegister->registerService(
                        ZeroconfRecord(tr("%1").arg(localIP),
                        QLatin1String(m_ServerServiceName), QString()),
                        m_zeroconfServer.serverPort());
                }
            }
            else {
                m_pZeroconfRegister->registerService(
                    ZeroconfRecord(tr("%1").arg(m_pMainWindow->getScreenName()),
                    QLatin1String(m_ClientServiceName), QString()),
                    m_zeroconfServer.serverPort());
            }

            m_ServiceRegistered = true;
        }
    }

    return result;
}
