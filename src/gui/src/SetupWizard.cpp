/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
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

#include "SetupWizard.h"
#include "ui_SetupWizard.h"
#include "MainWindow.h"
#include "QInputLeapApplication.h"
#include "QUtility.h"

#include <QMessageBox>

SetupWizard::SetupWizard(MainWindow& mainWindow, bool startMain) :
    ui_{std::make_unique<Ui::SetupWizard>()},
    m_MainWindow(mainWindow),
    m_StartMain(startMain)
{
    ui_->setupUi(this);

#if defined(Q_OS_MAC)

    // the mac style needs a little more room because of the
    // graphic on the left.
    resize(600, 500);
    setMinimumSize(size());

#elif defined(Q_OS_WIN)

    // when aero is disabled on windows, the next/back buttons
    // are hidden (must be a qt bug) -- resizing the window
    // to +1 of the original height seems to fix this.
    // NOTE: calling setMinimumSize after this will break
    // it again, so don't do that.
    resize(size().width(), size().height() + 1);

#endif

    connect(ui_->m_pServerRadioButton, &QRadioButton::toggled, &m_MainWindow, &MainWindow::setServerMode);
    connect(ui_->m_pClientRadioButton, &QRadioButton::toggled, this, [=] (bool clientMode) {
        m_MainWindow.setServerMode(!clientMode);
    });

    m_Locale.fillLanguageComboBox(ui_->m_pComboLanguage);
    setIndexFromItemData(ui_->m_pComboLanguage, m_MainWindow.appConfig().language());
}

SetupWizard::~SetupWizard() = default;

bool SetupWizard::validateCurrentPage()
{
    QMessageBox message;
    message.setWindowTitle(tr("Setup InputLeap"));
    message.setIcon(QMessageBox::Information);

    if (currentPage() == ui_->m_pNodePage)
    {
        bool result = ui_->m_pClientRadioButton->isChecked() ||
                 ui_->m_pServerRadioButton->isChecked();

        if (!result)
        {
            message.setText(tr("Please select an option."));
            message.exec();
            return false;
        }
    }

    return true;
}

void SetupWizard::changeEvent(QEvent* event)
{
    if (event != nullptr)
    {
        switch (event->type())
        {
        case QEvent::LanguageChange:
            {
                ui_->m_pComboLanguage->blockSignals(true);
                ui_->retranslateUi(this);
                ui_->m_pComboLanguage->blockSignals(false);
                break;
            }

        default:
            QWizard::changeEvent(event);
        }
    }
}

void SetupWizard::accept()
{
    AppConfig& appConfig = m_MainWindow.appConfig();

    appConfig.setLanguage(ui_->m_pComboLanguage->itemData(ui_->m_pComboLanguage->currentIndex()).toString());

    appConfig.setWizardHasRun();
    appConfig.saveSettings();

    QSettings& settings = m_MainWindow.settings();
    if (ui_->m_pServerRadioButton->isChecked())
    {
        settings.setValue("groupServerChecked", true);
        settings.setValue("groupClientChecked", false);
    }
    if (ui_->m_pClientRadioButton->isChecked())
    {
        settings.setValue("groupClientChecked", true);
        settings.setValue("groupServerChecked", false);
    }

    QWizard::accept();

    if (m_StartMain)
    {
        m_MainWindow.updateZeroconfService();
        m_MainWindow.open();
    }
}

void SetupWizard::reject()
{
    QInputLeapApplication::getInstance()->switchTranslator(m_MainWindow.appConfig().language());

    if (m_StartMain)
    {
        m_MainWindow.open();
    }

    QWizard::reject();
}

void SetupWizard::on_m_pComboLanguage_currentIndexChanged(int index)
{
    QString ietfCode = ui_->m_pComboLanguage->itemData(index).toString();
    QInputLeapApplication::getInstance()->switchTranslator(ietfCode);
}
