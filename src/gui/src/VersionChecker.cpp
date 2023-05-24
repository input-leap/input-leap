/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2012 Nick Bolton
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

#include "VersionChecker.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QProcess>
#include <QLocale>
#include <QRegularExpression>

#define VERSION_REGEX "(\\d+\\.\\d+\\.\\d+)"
//#define VERSION_URL "http://www.TODO.com/"

VersionChecker::VersionChecker(QObject* parent)
    : QObject(parent)
{
}

void VersionChecker::checkLatest()
{
    // calling m_manager->get(..) is causing an access violation on app close
    // atm there is nothing to check the version against, so removing until we need a version checker again

    //m_manager = new QNetworkAccessManager(this);

    //connect(m_manager, SIGNAL(finished(QNetworkReply*)),
    //    this, SLOT(replyFinished(QNetworkReply*)));

    //m_manager->get(QNetworkRequest(QUrl(VERSION_URL)));
}

void VersionChecker::replyFinished(QNetworkReply* reply)
{
    if (reply->error()) {
        // TODO: handle me
    } else {
        QString newestVersion = QString(reply->readAll());
        if (!newestVersion.isEmpty()) {
            QString currentVersion = getVersion();
            if (currentVersion != "Unknown") {
                if (compareVersions(currentVersion, newestVersion) > 0)
                    emit updateFound(newestVersion);
            }
        }
    }
    reply->deleteLater();
}

int VersionChecker::compareVersions(const QString& left, const QString& right)
{
    if (left.compare(right) == 0)
        return 0; // versions are same.

    QStringList leftSplit = left.split(QRegularExpression("\\."));
    if (leftSplit.size() != 3)
        return 1; // assume right wins.

    QStringList rightSplit = right.split(QRegularExpression("\\."));
    if (rightSplit.size() != 3)
        return -1; // assume left wins.

    int leftMajor = leftSplit.at(0).toInt();
    int leftMinor = leftSplit.at(1).toInt();
    int leftRev = leftSplit.at(2).toInt();

    int rightMajor = rightSplit.at(0).toInt();
    int rightMinor = rightSplit.at(1).toInt();
    int rightRev = rightSplit.at(2).toInt();

    bool rightWins =
        (rightMajor > leftMajor) ||
        ((rightMajor >= leftMajor) && (rightMinor > leftMinor)) ||
        ((rightMajor >= leftMajor) && (rightMinor >= leftMinor) && (rightRev > leftRev));

    return rightWins ? 1 : -1;
}

QString VersionChecker::getVersion()
{
    QProcess process;
    process.start(m_app, QStringList() << "--version");

    process.setReadChannel(QProcess::StandardOutput);
    if (process.waitForStarted() && process.waitForFinished())
    {
        QRegularExpression rx(VERSION_REGEX);
        QString text = process.readLine();
        QRegularExpressionMatch match = rx.match(text);
        if (match.hasMatch())
        {
            return match.captured(1);
        }
    }

    return tr("Unknown");
}

