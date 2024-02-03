/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2008 Volker Lanz (vl@fidra.de)
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

#include "AppLocale.h"

class AppConfig;

namespace Ui
{
    class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

    public:
        SettingsDialog(QWidget* parent, AppConfig& config);
        ~SettingsDialog() override;

    protected:
        void accept() override;
        void reject() override;
        void changeEvent(QEvent* event) override;
        AppConfig& appConfig() { return m_appConfig; }

    private:
        std::unique_ptr<Ui::SettingsDialog> ui_;
        AppConfig& m_appConfig;
        AppLocale m_Locale;

    private slots:
        void on_m_pComboLanguage_currentIndexChanged(int index);
        void on_m_pCheckBoxLogToFile_stateChanged(int );
        void on_m_pButtonBrowseLog_clicked();
};
