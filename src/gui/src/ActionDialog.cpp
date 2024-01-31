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

#include "ActionDialog.h"
#include "ui_ActionDialog.h"

#include "Hotkey.h"
#include "Action.h"
#include "ServerConfig.h"
#include "KeySequence.h"

#include <QtCore>
#include <QtGui>
#include <QButtonGroup>

ActionDialog::ActionDialog(QWidget* parent, ServerConfig& config, Hotkey& hotkey, Action& action) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::ActionDialog>()},
    m_ServerConfig(config),
    m_Hotkey(hotkey),
    m_Action(action),
    m_pButtonGroupType(new QButtonGroup(this))
{
    ui_->setupUi(this);

    // work around Qt Designer's lack of a QButtonGroup; we need it to get
    // at the button id of the checked radio button
    QRadioButton* const typeButtons[] = { ui_->m_pRadioPress, ui_->m_pRadioRelease, ui_->m_pRadioPressAndRelease, ui_->m_pRadioSwitchToScreen, ui_->m_pRadioToggleScreen, ui_->m_pRadioSwitchInDirection, ui_->m_pRadioLockCursorToScreen };

    for (unsigned int i = 0; i < sizeof(typeButtons) / sizeof(typeButtons[0]); i++)
        m_pButtonGroupType->addButton(typeButtons[i], i);

    ui_->m_pKeySequenceWidgetHotkey->setText(m_Action.keySequence().toString());
    ui_->m_pKeySequenceWidgetHotkey->setKeySequence(m_Action.keySequence());
    m_pButtonGroupType->button(m_Action.type())->setChecked(true);
    ui_->m_pComboSwitchInDirection->setCurrentIndex(m_Action.switchDirection());
    ui_->m_pComboLockCursorToScreen->setCurrentIndex(m_Action.lockCursorMode());

    if (m_Action.activeOnRelease())
        ui_->m_pRadioHotkeyReleased->setChecked(true);
    else
        ui_->m_pRadioHotkeyPressed->setChecked(true);

    ui_->m_pGroupBoxScreens->setChecked(m_Action.haveScreens());

    int idx = 0;
    for (const Screen& screen : serverConfig().screens()) {
        if (!screen.isNull())
        {
            QListWidgetItem *pListItem = new QListWidgetItem(screen.name());
            ui_->m_pListScreens->addItem(pListItem);
            if (m_Action.typeScreenNames().indexOf(screen.name()) != -1)
                ui_->m_pListScreens->setCurrentItem(pListItem);

            ui_->m_pComboSwitchToScreen->addItem(screen.name());
            if (screen.name() == m_Action.switchScreenName())
                ui_->m_pComboSwitchToScreen->setCurrentIndex(idx);

            idx++;
        }
    }
}

void ActionDialog::accept()
{
    if (!sequenceWidget()->valid() && m_pButtonGroupType->checkedId() >= 0 && m_pButtonGroupType->checkedId() < 3)
        return;

    m_Action.setKeySequence(sequenceWidget()->keySequence());
    m_Action.setType(m_pButtonGroupType->checkedId());
    m_Action.setHaveScreens(ui_->m_pGroupBoxScreens->isChecked());

    m_Action.clearTypeScreenNames();
    const auto selection = ui_->m_pListScreens->selectedItems();
    for (const QListWidgetItem* pItem : selection) {
        m_Action.appendTypeScreenName(pItem->text());
    }

    m_Action.setSwitchScreenName(ui_->m_pComboSwitchToScreen->currentText());
    m_Action.setSwitchDirection(ui_->m_pComboSwitchInDirection->currentIndex());
    m_Action.setLockCursorMode(ui_->m_pComboLockCursorToScreen->currentIndex());
    m_Action.setActiveOnRelease(ui_->m_pRadioHotkeyReleased->isChecked());

    QDialog::accept();
}

void ActionDialog::on_m_pKeySequenceWidgetHotkey_keySequenceChanged()
{
    if (sequenceWidget()->keySequence().isMouseButton())
    {
        ui_->m_pGroupBoxScreens->setEnabled(false);
        ui_->m_pListScreens->setEnabled(false);
    }
    else
    {
        ui_->m_pGroupBoxScreens->setEnabled(true);
        ui_->m_pListScreens->setEnabled(true);
    }
}

const KeySequenceWidget *ActionDialog::sequenceWidget() const
{
    return ui_->m_pKeySequenceWidgetHotkey;
}

ActionDialog::~ActionDialog() = default;

