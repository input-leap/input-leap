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

#if !defined(LOGWINDOW__H)

#define LOGWINDOW__H

#include <QDialog>

#include "ui_LogWindow.h"

namespace Ui
{
    class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT

    public:
        LogWindow(QWidget *parent);

        void startNewInstance();

        void appendRaw(const QString& text = QString());
        void appendInfo(const QString& text = QString());
        void appendDebug(const QString& text = QString());
        void appendError(const QString& text = QString());

    private:
        Ui::LogWindow *ui = nullptr;
        static const QString m_error;
        static const QString m_info;
        static const QString m_debug;
};

#endif // LOGWINDOW__H
