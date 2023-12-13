/*
* InputLeap -- mouse and keyboard sharing utility
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

#include <QTimer>

static QString getTimeStamp()
{
    QDateTime current = QDateTime::currentDateTime();
    return '[' + current.toString(Qt::ISODate) + ']';
}

LogWindow::LogWindow(QWidget *parent) :
    QDialog(parent)
{
    // explicitly unset DeleteOnClose so the log window can be show and hidden
    // repeatedly until InputLeap is finished
    setAttribute(Qt::WA_DeleteOnClose, false);
    setupUi(this);
    
    // Use a timer to flush the buffer every 100 milliseconds
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &LogWindow::flushBuffer);
    timer->start(100);
}

void LogWindow::startNewInstance()
{
    // put a space between last log output and new instance.
    if (!buffer.isEmpty())
        buffer += '\n';
}

void LogWindow::appendInfo(const QString& text)
{
    buffer += getTimeStamp() + " INFO: " + text + '\n';
}

void LogWindow::appendDebug(const QString& text)
{
    buffer += getTimeStamp() + " DEBUG: " + text + '\n';
}

void LogWindow::appendError(const QString& text)
{
    buffer += getTimeStamp() + " ERROR: " + text + '\n';
}

void LogWindow::appendRaw(const QString& text)
{
    buffer += text + '\n';
}

void LogWindow::flushBuffer()
{
    if (!buffer.isEmpty())
    {
        // Insert the new log content from the buffer
        m_pLogOutput->appendPlainText(buffer);
        buffer.clear();

        // Scroll to the bottom if it was at the bottom before
        QTimer::singleShot(0, this, [this]() {
            if (m_pLogOutput->textCursor().atEnd()) {
                QTextCursor cursor = m_pLogOutput->textCursor();
                cursor.movePosition(QTextCursor::End);
                m_pLogOutput->setTextCursor(cursor);
                m_pLogOutput->ensureCursorVisible();
            }
        });
    }
}

void LogWindow::on_m_pButtonHide_clicked()
{
    hide();
}

void LogWindow::on_m_pButtonClearLog_clicked()
{
    m_pLogOutput->clear();
}
