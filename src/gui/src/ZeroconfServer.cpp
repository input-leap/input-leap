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

#include "ZeroconfServer.h"
#include "ZeroconfThread.h"

#include <stdlib.h>

ZeroconfServer::ZeroconfServer(QObject* parent) :
    QTcpServer(parent)
{
}

void ZeroconfServer::incomingConnection(qintptr socketDescriptor)
{
    ZeroconfThread* thread = new ZeroconfThread(socketDescriptor, this);
    connect(thread, &ZeroconfThread::finished, thread, &ZeroconfThread::deleteLater);
    thread->start();
}
