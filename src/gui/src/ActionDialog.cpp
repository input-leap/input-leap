/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023-2024 InputLeap Developers
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

#include <QButtonGroup>

ActionDialog::ActionDialog(QWidget* parent, ServerConfig& config, Hotkey& hotkey, Action& action) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::ActionDialog>()},
    server_config_(config),
    hotkey_(hotkey),
    action_(action),
    button_group_type_(new QButtonGroup(this))
{
    ui_->setupUi(this);
    connect(ui_->keySequenceWidget, &KeySequenceWidget::keySequenceChanged, this, &ActionDialog::key_sequence_changed);
    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &ActionDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &ActionDialog::reject);

    // work around Qt Designer's lack of a QButtonGroup; we need it to get
    // at the button id of the checked radio button
    QRadioButton* const typeButtons[] = { ui_->m_pRadioPress, ui_->m_pRadioRelease, ui_->m_pRadioPressAndRelease, ui_->m_pRadioSwitchToScreen, ui_->m_pRadioToggleScreen, ui_->m_pRadioSwitchInDirection, ui_->m_pRadioLockCursorToScreen };

    for (unsigned int i = 0; i < sizeof(typeButtons) / sizeof(typeButtons[0]); i++)
        button_group_type_->addButton(typeButtons[i], i);

    ui_->keySequenceWidget->setText(action_.keySequence().toString());
    ui_->keySequenceWidget->setKeySequence(action_.keySequence());

    button_group_type_->button(action_.type())->setChecked(true);
    ui_->m_pComboSwitchInDirection->setCurrentIndex(action_.switchDirection());
    ui_->m_pComboLockCursorToScreen->setCurrentIndex(action_.lockCursorMode());

    if (action_.activeOnRelease())
        ui_->m_pRadioHotkeyReleased->setChecked(true);
    else
        ui_->m_pRadioHotkeyPressed->setChecked(true);

    ui_->m_pGroupBoxScreens->setChecked(action_.haveScreens());

    int idx = 0;
    for (const Screen& screen : serverConfig().screens()) {
        if (!screen.isNull())
        {
            QListWidgetItem *pListItem = new QListWidgetItem(screen.name());
            ui_->m_pListScreens->addItem(pListItem);
            if (action_.typeScreenNames().indexOf(screen.name()) != -1)
                ui_->m_pListScreens->setCurrentItem(pListItem);

            ui_->m_pComboSwitchToScreen->addItem(screen.name());
            if (screen.name() == action_.switchScreenName())
                ui_->m_pComboSwitchToScreen->setCurrentIndex(idx);

            idx++;
        }
    }
}

void ActionDialog::accept()
{
    if (!ui_->keySequenceWidget->valid() && button_group_type_->checkedId() >= 0 &&
            button_group_type_->checkedId() < 3) {
        return;
    }

    action_.setKeySequence(ui_->keySequenceWidget->keySequence());
    action_.setType(button_group_type_->checkedId());
    action_.setHaveScreens(ui_->m_pGroupBoxScreens->isChecked());

    action_.clearTypeScreenNames();
    const auto selection = ui_->m_pListScreens->selectedItems();
    for (const QListWidgetItem* pItem : selection) {
        action_.appendTypeScreenName(pItem->text());
    }

    action_.setSwitchScreenName(ui_->m_pComboSwitchToScreen->currentText());
    action_.setSwitchDirection(ui_->m_pComboSwitchInDirection->currentIndex());
    action_.setLockCursorMode(ui_->m_pComboLockCursorToScreen->currentIndex());
    action_.setActiveOnRelease(ui_->m_pRadioHotkeyReleased->isChecked());

    QDialog::accept();
}

void ActionDialog::key_sequence_changed()
{
    if (ui_->keySequenceWidget->keySequence().isMouseButton())
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

ActionDialog::~ActionDialog() = default;

