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

#include "ServerConfigDialog.h"
#include <ui_ServerConfigDialog.h>

#include "ServerConfig.h"
#include "HotkeyDialog.h"
#include "ActionDialog.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>

ServerConfigDialog::ServerConfigDialog(QWidget* parent, ServerConfig& config, const QString& defaultScreenName) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::ServerConfigDialog>()},
    m_OrigServerConfig(config),
    m_ServerConfig(config),
    m_ScreenSetupModel(serverConfig().screens(), serverConfig().numColumns(), serverConfig().numRows()),
    m_Message("")
{
    ui_->setupUi(this);

    ui_->m_pCheckBoxHeartbeat->setChecked(serverConfig().hasHeartbeat());
    ui_->m_pSpinBoxHeartbeat->setValue(serverConfig().heartbeat());

    ui_->m_pCheckBoxRelativeMouseMoves->setChecked(serverConfig().relativeMouseMoves());
    ui_->m_pCheckBoxScreenSaverSync->setChecked(serverConfig().screenSaverSync());
    ui_->m_pCheckBoxWin32KeepForeground->setChecked(serverConfig().win32KeepForeground());

    ui_->m_pCheckBoxSwitchDelay->setChecked(serverConfig().hasSwitchDelay());
    ui_->m_pSpinBoxSwitchDelay->setValue(serverConfig().switchDelay());

    ui_->m_pCheckBoxSwitchDoubleTap->setChecked(serverConfig().hasSwitchDoubleTap());
    ui_->m_pSpinBoxSwitchDoubleTap->setValue(serverConfig().switchDoubleTap());

    ui_->m_pCheckBoxCornerTopLeft->setChecked(serverConfig().switchCorner(BaseConfig::SwitchCorner::TopLeft));
    ui_->m_pCheckBoxCornerTopRight->setChecked(serverConfig().switchCorner(BaseConfig::SwitchCorner::TopRight));
    ui_->m_pCheckBoxCornerBottomLeft->setChecked(serverConfig().switchCorner(BaseConfig::SwitchCorner::BottomLeft));
    ui_->m_pCheckBoxCornerBottomRight->setChecked(serverConfig().switchCorner(BaseConfig::SwitchCorner::BottomRight));
    ui_->m_pSpinBoxSwitchCornerSize->setValue(serverConfig().switchCornerSize());

    ui_->m_pCheckBoxIgnoreAutoConfigClient->setChecked(serverConfig().ignoreAutoConfigClient());

    ui_->m_pCheckBoxEnableDragAndDrop->setChecked(serverConfig().enableDragAndDrop());

    ui_->m_pCheckBoxEnableClipboard->setChecked(serverConfig().clipboardSharing());
    ui_->m_pSpinBoxClipboardSizeLimit->setValue(serverConfig().clipboardSharingSize());
    ui_->m_pSpinBoxClipboardSizeLimit->setEnabled(serverConfig().clipboardSharing());

    for (const Hotkey& hotkey : serverConfig().hotkeys()) {
        ui_->m_pListHotkeys->addItem(hotkey.text());
    }

    ui_->m_pScreenSetupView->setModel(&m_ScreenSetupModel);

    if (serverConfig().numScreens() == 0)
        model().screen(serverConfig().numColumns() / 2, serverConfig().numRows() / 2) = Screen(defaultScreenName);
}

void ServerConfigDialog::showEvent(QShowEvent* event)
{
    (void) event;

    QDialog::show();

    if (!m_Message.isEmpty())
    {
        // TODO: ideally this message box should pop up after the dialog is shown
        QMessageBox::information(this, tr("Configure server"), m_Message);
    }
}

void ServerConfigDialog::accept()
{
    serverConfig().haveHeartbeat(ui_->m_pCheckBoxHeartbeat->isChecked());
    serverConfig().setHeartbeat(ui_->m_pSpinBoxHeartbeat->value());

    serverConfig().setRelativeMouseMoves(ui_->m_pCheckBoxRelativeMouseMoves->isChecked());
    serverConfig().setScreenSaverSync(ui_->m_pCheckBoxScreenSaverSync->isChecked());
    serverConfig().setWin32KeepForeground(ui_->m_pCheckBoxWin32KeepForeground->isChecked());

    serverConfig().haveSwitchDelay(ui_->m_pCheckBoxSwitchDelay->isChecked());
    serverConfig().setSwitchDelay(ui_->m_pSpinBoxSwitchDelay->value());

    serverConfig().haveSwitchDoubleTap(ui_->m_pCheckBoxSwitchDoubleTap->isChecked());
    serverConfig().setSwitchDoubleTap(ui_->m_pSpinBoxSwitchDoubleTap->value());

    serverConfig().setSwitchCorner(BaseConfig::SwitchCorner::TopLeft,
                                   ui_->m_pCheckBoxCornerTopLeft->isChecked());
    serverConfig().setSwitchCorner(BaseConfig::SwitchCorner::TopRight,
                                   ui_->m_pCheckBoxCornerTopRight->isChecked());
    serverConfig().setSwitchCorner(BaseConfig::SwitchCorner::BottomLeft,
                                   ui_->m_pCheckBoxCornerBottomLeft->isChecked());
    serverConfig().setSwitchCorner(BaseConfig::SwitchCorner::BottomRight,
                                   ui_->m_pCheckBoxCornerBottomRight->isChecked());
    serverConfig().setSwitchCornerSize(ui_->m_pSpinBoxSwitchCornerSize->value());
    serverConfig().setIgnoreAutoConfigClient(ui_->m_pCheckBoxIgnoreAutoConfigClient->isChecked());
    serverConfig().setEnableDragAndDrop(ui_->m_pCheckBoxEnableDragAndDrop->isChecked());
    serverConfig().setClipboardSharing(ui_->m_pCheckBoxEnableClipboard->isChecked());
    serverConfig().setClipboardSharingSize(ui_->m_pSpinBoxClipboardSizeLimit->value());

    // now that the dialog has been accepted, copy the new server config to the original one,
    // which is a reference to the one in MainWindow.
    setOrigServerConfig(serverConfig());

    QDialog::accept();
}

void ServerConfigDialog::on_m_pButtonNewHotkey_clicked()
{
    Hotkey hotkey;
    HotkeyDialog dlg(this, hotkey);
    if (dlg.exec() == QDialog::Accepted)
    {
        serverConfig().hotkeys().push_back(hotkey);
        ui_->m_pListHotkeys->addItem(hotkey.text());
    }
}

void ServerConfigDialog::on_m_pButtonEditHotkey_clicked()
{
    int idx = ui_->m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < static_cast<int>(serverConfig().hotkeys().size()));
    Hotkey& hotkey = serverConfig().hotkeys()[idx];
    HotkeyDialog dlg(this, hotkey);
    if (dlg.exec() == QDialog::Accepted)
        ui_->m_pListHotkeys->currentItem()->setText(hotkey.text());
}

void ServerConfigDialog::on_m_pButtonRemoveHotkey_clicked()
{
    int idx = ui_->m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < static_cast<int>(serverConfig().hotkeys().size()));
    serverConfig().hotkeys().erase(serverConfig().hotkeys().begin() + idx);
    ui_->m_pListActions->clear();
    delete ui_->m_pListHotkeys->item(idx);
}

void ServerConfigDialog::on_m_pListHotkeys_itemSelectionChanged()
{
    bool itemsSelected = !ui_->m_pListHotkeys->selectedItems().isEmpty();
    ui_->m_pButtonEditHotkey->setEnabled(itemsSelected);
    ui_->m_pButtonRemoveHotkey->setEnabled(itemsSelected);
    ui_->m_pButtonNewAction->setEnabled(itemsSelected);

    if (itemsSelected && serverConfig().hotkeys().size() > 0)
    {
        ui_->m_pListActions->clear();

        int idx = ui_->m_pListHotkeys->row(ui_->m_pListHotkeys->selectedItems()[0]);

        // There's a bug somewhere around here: We get idx == 1 right after we deleted the next to last item, so idx can
        // only possibly be 0. GDB shows we got called indirectly from the delete line in
        // on_m_pButtonRemoveHotkey_clicked() above, but the delete is of course necessary and seems correct.
        // The while() is a generalized workaround for all that and shouldn't be required.
        while (idx >= 0 && idx >= static_cast<int>(serverConfig().hotkeys().size()))
            idx--;

        Q_ASSERT(idx >= 0 && idx < static_cast<int>(serverConfig().hotkeys().size()));

        const Hotkey& hotkey = serverConfig().hotkeys()[idx];
        for (const Action& action : hotkey.actions()) {
            ui_->m_pListActions->addItem(action.text());
        }
    }
}

void ServerConfigDialog::on_m_pButtonNewAction_clicked()
{
    int idx = ui_->m_pListHotkeys->currentRow();
    Q_ASSERT(idx >= 0 && idx < static_cast<int>(serverConfig().hotkeys().size()));
    Hotkey& hotkey = serverConfig().hotkeys()[idx];

    Action action;
    ActionDialog dlg(this, serverConfig(), hotkey, action);
    if (dlg.exec() == QDialog::Accepted)
    {
        hotkey.appendAction(action);
        ui_->m_pListActions->addItem(action.text());
    }
}

void ServerConfigDialog::on_m_pButtonEditAction_clicked()
{
    int idxHotkey = ui_->m_pListHotkeys->currentRow();
    Q_ASSERT(idxHotkey >= 0 && idxHotkey < static_cast<int>(serverConfig().hotkeys().size()));
    Hotkey& hotkey = serverConfig().hotkeys()[idxHotkey];

    int idxAction = ui_->m_pListActions->currentRow();
    Q_ASSERT(idxAction >= 0 && idxAction < static_cast<int>(hotkey.actions().size()));
    Action action = hotkey.actions()[idxAction];

    ActionDialog dlg(this, serverConfig(), hotkey, action);
    if (dlg.exec() == QDialog::Accepted) {
        hotkey.setAction(idxAction, action);
        ui_->m_pListActions->currentItem()->setText(action.text());
    }
}

void ServerConfigDialog::on_m_pButtonRemoveAction_clicked()
{
    int idxHotkey = ui_->m_pListHotkeys->currentRow();
    Q_ASSERT(idxHotkey >= 0 && idxHotkey < static_cast<int>(serverConfig().hotkeys().size()));
    Hotkey& hotkey = serverConfig().hotkeys()[idxHotkey];

    int idxAction = ui_->m_pListActions->currentRow();
    Q_ASSERT(idxAction >= 0 && idxAction < static_cast<int>(hotkey.actions().size()));

    hotkey.removeAction(idxAction);
    delete ui_->m_pListActions->currentItem();
}

void ServerConfigDialog::on_m_pListActions_itemSelectionChanged()
{
    ui_->m_pButtonEditAction->setEnabled(!ui_->m_pListActions->selectedItems().isEmpty());
    ui_->m_pButtonRemoveAction->setEnabled(!ui_->m_pListActions->selectedItems().isEmpty());
}

void ServerConfigDialog::on_m_pCheckBoxEnableClipboard_stateChanged(int state)
{
    ui_->m_pSpinBoxClipboardSizeLimit->setEnabled(state == Qt::Checked);
}

ServerConfigDialog::~ServerConfigDialog() = default;
