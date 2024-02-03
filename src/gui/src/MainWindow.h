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

#include "inputleap/AppRole.h"
#include "AppConnectionState.h"

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QProcess>
#include <QThread>



#include "ServerConfig.h"
#include "AppConfig.h"
#include "IpcClient.h"
#include "Ipc.h"
#include "LogWindow.h"

#include <QMutex>
#include <memory>

class QAction;
class QMenu;
class QLineEdit;
class QGroupBox;
class QPushButton;
class QTextEdit;
class QComboBox;
class QTabWidget;
class QCheckBox;
class QRadioButton;
class QTemporaryFile;
class QMessageBox;
class QAbstractButton;

class LogDialog;
class QInputLeapApplication;
class SetupWizard;
class ZeroconfService;
class DataDownloader;
class CommandProcess;
class SslCertificate;

namespace Ui
{
    class MainWindow;
}
class MainWindow : public QMainWindow
{
    Q_OBJECT

    friend class QInputLeapApplication;
    friend class SetupWizard;
    friend class SettingsDialog;

    public:
        enum qLevel {
            Error,
            Info
        };

        enum qRuningState {
            kStarted,
            kStopped
        };

    public:
        MainWindow(QSettings& settings, AppConfig& appConfig);
        ~MainWindow() override;

    public:
        void setVisible(bool visible) override;
        AppRole app_role() const;
        AppConnectionState connection_state() const { return connection_state_; }
        QString hostname() const;
        QString configFilename();
        QString address();
        QString appPath(const QString& name);
        void open();
        QString getScreenName();
        ServerConfig& serverConfig() { return m_ServerConfig; }
        void showConfigureServer(const QString& message);
        void showConfigureServer() { showConfigureServer(""); }
        void autoAddScreen(const QString name);
        void updateZeroconfService();
        void serverDetected(const QString name);

public slots:
        void appendLogRaw(const QString& text);
        void appendLogInfo(const QString& text);
        void appendLogDebug(const QString& text);
        void appendLogError(const QString& text);
        void start_cmd_app();
        void setServerMode(bool isServerMode);

    protected slots:
        void on_m_pGroupClient_toggled(bool on);
        void on_m_pGroupServer_toggled(bool on);
        bool on_m_pButtonBrowseConfigFile_clicked();
        void on_m_pButtonConfigureServer_clicked();
        bool on_m_pActionSave_triggered();
        void on_m_pActionAbout_triggered();
        void on_m_pActionSettings_triggered();
        void cmd_app_finished(int exitCode, QProcess::ExitStatus);
        void trayActivated(QSystemTrayIcon::ActivationReason reason);
        void stop_cmd_app();
        void logOutput();
        void logError();
        void bonjourInstallFinished();
        void showLogWindow();

    protected:
        QSettings& settings() { return m_Settings; }
        AppConfig& appConfig() { return *m_AppConfig; }
        void initConnections();
        void createMenuBar();
        void createTrayIcon();
        void loadSettings();
        void saveSettings();
        void set_icon(AppConnectionState state);
        void set_connection_state(AppConnectionState state);
        bool clientArgs(QStringList& args, QString& app);
        bool serverArgs(QStringList& args, QString& app);
        void setStatus(const QString& status);
        void updateFromLogLine(const QString& line);
        QString getIPAddresses();
        void stopService();
        void stopDesktop();
        void changeEvent(QEvent* event) override;
        bool event(QEvent* event) override;
        void retranslateMenuBar();
#if defined(Q_OS_WIN)
        bool isServiceRunning(QString name);
#else
        bool isServiceRunning();
#endif
        bool isBonjourRunning();
        void downloadBonjour();
        void promptAutoConfig();
        void checkConnected(const QString& line);
        void checkFingerprint(const QString& line);
        void restart_cmd_app();
        void proofreadInfo();
        void windowStateChanged();
        void updateSSLFingerprint();

    private:
        std::unique_ptr<Ui::MainWindow> ui_;
        QSettings& m_Settings;
        AppConfig* m_AppConfig;
        QProcess* cmd_app_process_;
        AppConnectionState connection_state_ = AppConnectionState::DISCONNECTED;
        ServerConfig m_ServerConfig;
        QTemporaryFile* m_pTempConfigFile;
        QSystemTrayIcon* m_pTrayIcon;
        QMenu* m_pTrayIconMenu;
        bool m_AlreadyHidden;
        IpcClient m_IpcClient;
        QMenuBar* m_pMenuBar;
        QMenu* main_menu_;
        QMenu* m_pMenuHelp;
        ZeroconfService* m_pZeroconfService;
        DataDownloader* m_pDataDownloader;
        QMessageBox* m_DownloadMessageBox;
        QAbstractButton* m_pCancelButton;
        QMutex m_UpdateZeroconfMutex;
        bool m_SuppressAutoConfigWarning;
        CommandProcess* m_BonjourInstall;
        bool m_SuppressEmptyServerWarning;
        qRuningState m_ExpectedRunningState;
        QMutex m_StopDesktopMutex;
        SslCertificate* m_pSslCertificate;
        QStringList m_PendingClientNames;
        LogWindow *m_pLogWindow;

        bool m_fingerprint_expanded = false;

private slots:
    void on_m_pCheckBoxAutoConfig_toggled(bool checked);
    void on_m_pComboServerList_currentIndexChanged(QString );
    void on_m_pButtonReload_clicked();
    void installBonjour();

};
