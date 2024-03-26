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

#pragma once

#include <QDialog>
#include <memory>

namespace Ui
{
    class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

    public:
        LogWindow(QWidget *parent);
        ~LogWindow() override;

        void startNewInstance();

        void appendRaw(const QString& text);
        void appendInfo(const QString& text);
        void appendDebug(const QString& text);
        void appendError(const QString& text);

    private slots:
        void on_m_pButtonHide_clicked();
        void on_m_pButtonClearLog_clicked();

    private:
        std::unique_ptr<Ui::LogWindow> ui_;
        QString buffer_;
        void flushBuffer();
};
