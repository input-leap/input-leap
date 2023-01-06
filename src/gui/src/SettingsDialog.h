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

#if !defined(SETTINGSDIALOG_H)

#define SETTINGSDIALOG_H

#include <QDialog>
#include "ui_SettingsDialogBase.h"
#include "BarrierLocale.h"

class AppConfig;

class SettingsDialog : public QDialog, public Ui::SettingsDialogBase
{
    Q_OBJECT

    public:
        SettingsDialog(QWidget* parent, AppConfig& config);

    protected:
        void accept() override;
        void reject() override;
        void changeEvent(QEvent* event) override;
        AppConfig& appConfig() { return m_appConfig; }

    private:
        AppConfig& m_appConfig;
        BarrierLocale m_Locale;

    private slots:
        void on_m_pComboLanguage_currentIndexChanged(int index);
        void on_m_pCheckBoxLogToFile_stateChanged(int );
        void on_m_pButtonBrowseLog_clicked();
};

#endif
