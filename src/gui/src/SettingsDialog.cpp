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

#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"

#include "AppLocale.h"
#include "QInputLeapApplication.h"
#include "QUtility.h"
#include "AppConfig.h"
#include "SslCertificate.h"
#include "MainWindow.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

SettingsDialog::SettingsDialog(QWidget* parent, AppConfig& config) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::SettingsDialog>()},
    m_appConfig(config)
{
    ui_->setupUi(this);

    m_Locale.fillLanguageComboBox(ui_->m_pComboLanguage);

    ui_->m_pLineEditScreenName->setText(appConfig().screenName());
    ui_->m_pSpinBoxPort->setValue(appConfig().port());
    ui_->m_pLineEditInterface->setText(appConfig().networkInterface());
    ui_->m_pComboLogLevel->setCurrentIndex(appConfig().logLevel());
    ui_->m_pCheckBoxLogToFile->setChecked(appConfig().logToFile());
    ui_->m_pLineEditLogFilename->setText(appConfig().logFilename());
    setIndexFromItemData(ui_->m_pComboLanguage, appConfig().language());
    ui_->m_pCheckBoxAutoHide->setChecked(appConfig().getAutoHide());
    ui_->m_pCheckBoxAutoStart->setChecked(appConfig().getAutoStart());
    ui_->m_pCheckBoxMinimizeToTray->setChecked(appConfig().getMinimizeToTray());
    ui_->m_pCheckBoxEnableCrypto->setChecked(m_appConfig.getCryptoEnabled());
    ui_->checkbox_require_client_certificate->setChecked(m_appConfig.getRequireClientCertificate());

#if defined(Q_OS_WIN)
    ui_->m_pComboElevate->setCurrentIndex(static_cast<int>(appConfig().elevateMode()));
#else
    // elevate checkbox is only useful on ms windows.
    ui_->m_pLabelElevate->hide();
    ui_->m_pComboElevate->hide();
#endif
}

void SettingsDialog::accept()
{
    m_appConfig.setScreenName(ui_->m_pLineEditScreenName->text());
    m_appConfig.setPort(ui_->m_pSpinBoxPort->value());
    m_appConfig.setNetworkInterface(ui_->m_pLineEditInterface->text());
    m_appConfig.setCryptoEnabled(ui_->m_pCheckBoxEnableCrypto->isChecked());
    m_appConfig.setRequireClientCertificate(ui_->checkbox_require_client_certificate->isChecked());
    m_appConfig.setLogLevel(ui_->m_pComboLogLevel->currentIndex());
    m_appConfig.setLogToFile(ui_->m_pCheckBoxLogToFile->isChecked());
    m_appConfig.setLogFilename(ui_->m_pLineEditLogFilename->text());
    m_appConfig.setLanguage(ui_->m_pComboLanguage->itemData(ui_->m_pComboLanguage->currentIndex()).toString());
    m_appConfig.setElevateMode(static_cast<ElevateMode>(ui_->m_pComboElevate->currentIndex()));
    m_appConfig.setAutoHide(ui_->m_pCheckBoxAutoHide->isChecked());
    m_appConfig.setAutoStart(ui_->m_pCheckBoxAutoStart->isChecked());
    m_appConfig.setMinimizeToTray(ui_->m_pCheckBoxMinimizeToTray->isChecked());
    m_appConfig.saveSettings();
    QDialog::accept();
}

void SettingsDialog::reject()
{
    if (m_appConfig.language() != ui_->m_pComboLanguage->itemData(ui_->m_pComboLanguage->currentIndex()).toString()) {
        QInputLeapApplication::getInstance()->switchTranslator(m_appConfig.language());
    }
    QDialog::reject();
}

void SettingsDialog::changeEvent(QEvent* event)
{
    if (event != nullptr)
    {
        switch (event->type())
        {
        case QEvent::LanguageChange:
            {
                int logLevelIndex = ui_->m_pComboLogLevel->currentIndex();

                ui_->m_pComboLanguage->blockSignals(true);
                ui_->retranslateUi(this);
                ui_->m_pComboLanguage->blockSignals(false);

                ui_->m_pComboLogLevel->setCurrentIndex(logLevelIndex);
                break;
            }

        default:
            QDialog::changeEvent(event);
        }
    }
}

void SettingsDialog::on_m_pCheckBoxLogToFile_stateChanged(int i)
{
    bool checked = i == 2;

    ui_->m_pLineEditLogFilename->setEnabled(checked);
    ui_->m_pButtonBrowseLog->setEnabled(checked);
}

void SettingsDialog::on_m_pButtonBrowseLog_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save log file to..."),
        ui_->m_pLineEditLogFilename->text(),
        "Logs (*.log *.txt)");

    if (!fileName.isEmpty())
    {
        ui_->m_pLineEditLogFilename->setText(fileName);
    }
}

void SettingsDialog::on_m_pComboLanguage_currentIndexChanged(int index)
{
    QString ietfCode = ui_->m_pComboLanguage->itemData(index).toString();
    QInputLeapApplication::getInstance()->switchTranslator(ietfCode);
}

SettingsDialog::~SettingsDialog() = default;
