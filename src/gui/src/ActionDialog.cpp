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

ActionDialog::ActionDialog(QWidget* parent, const ServerConfig& config, Hotkey& hotkey, Action& action) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::ActionDialog>()},
    hotkey_(hotkey),
    action_(action)
{
    ui_->setupUi(this);
    connect(ui_->keySequenceWidget, &KeySequenceWidget::keySequenceChanged, this, &ActionDialog::key_sequence_changed);
    connect(ui_->comboActionType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ActionDialog::actionTypeChanged);
    connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &ActionDialog::accept);
    connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &ActionDialog::reject);

    ui_->keySequenceWidget->setText(action_.keySequence().toString());
    ui_->keySequenceWidget->setKeySequence(action_.keySequence());

    ui_->comboSwitchInDirection->setCurrentIndex(action_.switchDirection());
    ui_->comboLockCursorToScreen->setCurrentIndex(action_.lockCursorMode());

    ui_->comboTriggerOn->setCurrentIndex(action_.activeOnRelease());

    for (const Screen& screen : config.screens()) {
        if (screen.isNull())
            continue;
        auto *newListItem = new QListWidgetItem(screen.name());
        newListItem->setCheckState(Qt::Checked);
        if (action_.typeScreenNames().indexOf(screen.name()) != -1)
            newListItem->setCheckState(Qt::Unchecked);
        ui_->listScreens->addItem(newListItem);
        ui_->comboSwitchToScreen->addItem(tr("Switch to %1").arg(screen.name()));
        if(screen.name() == action.switchScreenName())
            ui_->comboSwitchToScreen->setCurrentIndex(ui_->comboSwitchToScreen->count() - 1);
    }
    ui_->keySequenceWidget->setVisible(false);
    ui_->group_screens->setVisible(false);
    ui_->listScreens->setEnabled(!ui_->keySequenceWidget->keySequence().isMouseButton());
    ui_->comboSwitchToScreen->setVisible(false);
    ui_->comboSwitchInDirection->setVisible(false);
    ui_->comboLockCursorToScreen->setVisible(false);
}

void ActionDialog::accept()
{
    if (!ui_->keySequenceWidget->valid() && ui_->comboActionType->currentIndex() >= 0 &&
            ui_->comboActionType->currentIndex() < 3) {
        return;
    }

    action_.setKeySequence(ui_->keySequenceWidget->keySequence());
    action_.setType(ui_->comboActionType->currentIndex());

    action_.clearTypeScreenNames();
    int screenCount = ui_->listScreens->count();
    for (int i = 0; i < ui_->listScreens->count(); i++) {
        const auto& item = ui_->listScreens->item(i);
        if (item->checkState() == Qt::Unchecked) {
            screenCount--;
            action_.appendTypeScreenName(item->text());
        }
    }
    action_.setHaveScreens(screenCount);


    action_.setSwitchScreenName(ui_->comboSwitchToScreen->currentText().remove(tr("Switch to ")));
    action_.setSwitchDirection(ui_->comboSwitchInDirection->currentIndex());
    action_.setLockCursorMode(ui_->comboLockCursorToScreen->currentIndex());
    action_.setActiveOnRelease(ui_->comboTriggerOn->currentIndex());

    QDialog::accept();
}

void ActionDialog::key_sequence_changed()
{
    ui_->listScreens->setEnabled(!ui_->keySequenceWidget->keySequence().isMouseButton());
    ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((ui_->keySequenceWidget->valid() && isKeyAction(ui_->comboActionType->currentIndex())) || isKeyAction(ui_->comboActionType->currentIndex()));
}

void ActionDialog::actionTypeChanged(int index)
{
    ui_->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((ui_->keySequenceWidget->valid() && isKeyAction(index)) || !isKeyAction(index));
    ui_->keySequenceWidget->setVisible(isKeyAction(index));
    ui_->group_screens->setVisible(isKeyAction(index));
    ui_->listScreens->setEnabled(!ui_->keySequenceWidget->keySequence().isMouseButton());
    ui_->comboSwitchToScreen->setVisible(index == ACTIONTYPES::ACTION_SWITCH_TO);
    ui_->comboSwitchInDirection->setVisible(index == ACTIONTYPES::ACTION_SWITCH_IN_DIR);
    ui_->comboLockCursorToScreen->setVisible(index == ACTIONTYPES::ACTION_MODIFY_CURSOR_LOCK);
    adjustSize();
}

bool ActionDialog::isKeyAction(int index)
{
    return ((index == ACTIONTYPES::ACTION_PRESS_KEY) || (index == ACTIONTYPES::ACTION_RELEASE_KEY) || (index == ACTIONTYPES::ACTION_TOGGLE_KEY));
}

ActionDialog::~ActionDialog() = default;
