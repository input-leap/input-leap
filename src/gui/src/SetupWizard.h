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

#include "ui_SetupWizardBase.h"
#include "BarrierLocale.h"

#include <QWizard>
#include <QNetworkAccessManager>

class MainWindow;

class SetupWizard : public QWizard, public Ui::SetupWizardBase
{
    Q_OBJECT
public:
    enum {
        kMaximiumLoginAttemps = 3
    };

public:
    SetupWizard(MainWindow& mainWindow, bool startMain);
    virtual ~SetupWizard() override;
    bool validateCurrentPage() override;

protected:
    void changeEvent(QEvent* event) override;
    void accept() override;
    void reject() override;

private:
    MainWindow& m_MainWindow;
    bool m_StartMain;
    BarrierLocale m_Locale;

private slots:
    void on_m_pComboLanguage_currentIndexChanged(int index);
};
