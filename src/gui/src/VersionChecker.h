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

#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

class VersionChecker : public QObject
{
    Q_OBJECT
public:
    explicit VersionChecker(QObject* parent = nullptr);
    void checkLatest();
    QString getVersion();
    void setApp(const QString& app) { m_app = app; }
    int compareVersions(const QString& left, const QString& right);
public slots:
    void replyFinished(QNetworkReply* reply);
signals:
    void updateFound(const QString& version);
private:
    QNetworkAccessManager* m_manager;
    QString m_app;
};
