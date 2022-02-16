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

#pragma once

#include <QObject>
#include <string>
#include "io/filesystem.h"

class SslCertificate : public QObject
{
    Q_OBJECT

public:
    explicit SslCertificate(QObject *parent = nullptr);

public slots:
    void generateCertificate();

signals:
    void error(QString e);
    void info(QString i);
    void generateFinished();

private:
    void generate_fingerprint(const inputleap::fs::path& cert_path);

    bool is_certificate_valid(const inputleap::fs::path& path);
};
