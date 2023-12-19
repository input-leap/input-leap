/*
* InputLeap -- mouse and keyboard sharing utility
* Copyright (C) 2023 InputLeap Developers
* Copyright (C) 2018 Debauchee Open Source Group
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

#include "LogWindow.h"

#include <QDateTime>

const QString LogWindow::m_debug = QStringLiteral("DEBUG: %1");
const QString LogWindow::m_error = QStringLiteral("ERROR: %1");
const QString LogWindow::m_info = QStringLiteral("INFO: %1");

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogWindow)
{
    // explicitly unset DeleteOnClose so the log window can be show and hidden
    // repeatedly until InputLeap is finished
    setAttribute(Qt::WA_DeleteOnClose, false);
    ui->setupUi(this);

    connect(ui->btnHide, &QPushButton::clicked, this, &QWidget::hide);
    connect(ui->btnClear, &QPushButton::clicked, ui->logOutput, &QTextEdit::clear);
}

void LogWindow::startNewInstance()
{
    // put a space between last log output and new instance.
    if (!ui->logOutput->toPlainText().isEmpty())
        ui->logOutput->append(QString());
}

void LogWindow::appendInfo(const QString& text)
{
    appendRaw(m_info.arg(text));
}

void LogWindow::appendDebug(const QString& text)
{
    appendRaw(m_debug.arg(text));
}

void LogWindow::appendError(const QString& text)
{
    appendRaw(m_error.arg(text));
}

void LogWindow::appendRaw(const QString& text)
{
    ui->logOutput->append(QStringLiteral("[%1] %2").arg(QDateTime::currentDateTime().toString(Qt::ISODate), text));
}
