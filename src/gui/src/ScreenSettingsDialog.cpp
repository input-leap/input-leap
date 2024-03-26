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

#include "ScreenSettingsDialog.h"
#include "ui_ScreenSettingsDialog.h"

#include "Screen.h"

#include <QtCore>
#include <QtGui>
#include <QMessageBox>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
static const QRegularExpression ValidScreenName("[a-z0-9\\._-]{,255}",
                                                QRegularExpression::CaseInsensitiveOption);
#else
static const QRegExp ValidScreenName("[a-z0-9\\._-]{,255}", Qt::CaseInsensitive);
#endif

static QString check_name_param(QString name)
{
    // after internationalization happens the default name "Unnamed" might
    // be translated with spaces (or other chars). let's replace the spaces
    // with dashes and just give up if that doesn't pass the regexp
    name.replace(' ', '-');
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    if (ValidScreenName.match(name).isValid())
#else
    if (ValidScreenName.exactMatch(name))
#endif
        return name;
    return "";
}

ScreenSettingsDialog::ScreenSettingsDialog(QWidget* parent, Screen* pScreen) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::ScreenSettingsDialog>()},
    m_pScreen(pScreen)
{
    ui_->setupUi(this);

    ui_->m_pLineEditName->setText(check_name_param(m_pScreen->name()));
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ui_->m_pLineEditName->setValidator(new QRegularExpressionValidator(ValidScreenName, ui_->m_pLineEditName));
#else
    ui_->m_pLineEditName->setValidator(new QRegExpValidator(ValidScreenName, ui_->m_pLineEditName));
#endif
    ui_->m_pLineEditName->selectAll();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    ui_->m_pLineEditAlias->setValidator(new QRegularExpressionValidator(ValidScreenName, ui_->m_pLineEditName));
#else
    ui_->m_pLineEditAlias->setValidator(new QRegExpValidator(ValidScreenName, ui_->m_pLineEditName));
#endif

    for (int i = 0; i < m_pScreen->aliases().count(); i++)
        new QListWidgetItem(m_pScreen->aliases()[i], ui_->m_pListAliases);

    ui_->m_pComboBoxShift->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Shift)));
    ui_->m_pComboBoxCtrl->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Ctrl)));
    ui_->m_pComboBoxAlt->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Alt)));
    ui_->m_pComboBoxMeta->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Meta)));
    ui_->m_pComboBoxSuper->setCurrentIndex(static_cast<int>(m_pScreen->modifier(Screen::Modifier::Super)));

    ui_->m_pCheckBoxCornerTopLeft->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::TopLeft));
    ui_->m_pCheckBoxCornerTopRight->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::TopRight));
    ui_->m_pCheckBoxCornerBottomLeft->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::BottomLeft));
    ui_->m_pCheckBoxCornerBottomRight->setChecked(m_pScreen->switchCorner(Screen::SwitchCorner::BottomRight));
    ui_->m_pSpinBoxSwitchCornerSize->setValue(m_pScreen->switchCornerSize());

    ui_->m_pCheckBoxCapsLock->setChecked(m_pScreen->fix(Screen::Fix::CapsLock));
    ui_->m_pCheckBoxNumLock->setChecked(m_pScreen->fix(Screen::Fix::NumLock));
    ui_->m_pCheckBoxScrollLock->setChecked(m_pScreen->fix(Screen::Fix::ScrollLock));
    ui_->m_pCheckBoxXTest->setChecked(m_pScreen->fix(Screen::Fix::XTest));
    ui_->m_pCheckBoxPreserveFocus->setChecked(m_pScreen->fix(Screen::Fix::PreserveFocus));
}

void ScreenSettingsDialog::accept()
{
    if (ui_->m_pLineEditName->text().isEmpty())
    {
        QMessageBox::warning(
            this, tr("Screen name is empty"),
            tr("The screen name cannot be empty. "
               "Please either fill in a name or cancel the dialog."));
        return;
    }

    m_pScreen->init();

    m_pScreen->setName(ui_->m_pLineEditName->text());

    for (int i = 0; i < ui_->m_pListAliases->count(); i++)
    {
        QString alias(ui_->m_pListAliases->item(i)->text());
        if (alias == ui_->m_pLineEditName->text())
        {
            QMessageBox::warning(
                this, tr("Screen name matches alias"),
                tr("The screen name cannot be the same as an alias. "
                   "Please either remove the alias or change the screen name."));
            return;
        }
        m_pScreen->addAlias(alias);
    }

    m_pScreen->setModifier(Screen::Modifier::Shift,
                           static_cast<Screen::Modifier>(ui_->m_pComboBoxShift->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Ctrl,
                           static_cast<Screen::Modifier>(ui_->m_pComboBoxCtrl->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Alt,
                           static_cast<Screen::Modifier>(ui_->m_pComboBoxAlt->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Meta,
                           static_cast<Screen::Modifier>(ui_->m_pComboBoxMeta->currentIndex()));
    m_pScreen->setModifier(Screen::Modifier::Super,
                           static_cast<Screen::Modifier>(ui_->m_pComboBoxSuper->currentIndex()));

    m_pScreen->setSwitchCorner(Screen::SwitchCorner::TopLeft, ui_->m_pCheckBoxCornerTopLeft->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::TopRight, ui_->m_pCheckBoxCornerTopRight->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::BottomLeft, ui_->m_pCheckBoxCornerBottomLeft->isChecked());
    m_pScreen->setSwitchCorner(Screen::SwitchCorner::BottomRight, ui_->m_pCheckBoxCornerBottomRight->isChecked());
    m_pScreen->setSwitchCornerSize(ui_->m_pSpinBoxSwitchCornerSize->value());

    m_pScreen->setFix(Screen::Fix::CapsLock, ui_->m_pCheckBoxCapsLock->isChecked());
    m_pScreen->setFix(Screen::Fix::NumLock, ui_->m_pCheckBoxNumLock->isChecked());
    m_pScreen->setFix(Screen::Fix::ScrollLock, ui_->m_pCheckBoxScrollLock->isChecked());
    m_pScreen->setFix(Screen::Fix::XTest, ui_->m_pCheckBoxXTest->isChecked());
    m_pScreen->setFix(Screen::Fix::PreserveFocus, ui_->m_pCheckBoxPreserveFocus->isChecked());

    QDialog::accept();
}

void ScreenSettingsDialog::on_m_pButtonAddAlias_clicked()
{
    if (!ui_->m_pLineEditAlias->text().isEmpty() && ui_->m_pListAliases->findItems(ui_->m_pLineEditAlias->text(), Qt::MatchFixedString).isEmpty())
    {
        new QListWidgetItem(ui_->m_pLineEditAlias->text(), ui_->m_pListAliases);
        ui_->m_pLineEditAlias->clear();
    }
}

void ScreenSettingsDialog::on_m_pLineEditAlias_textChanged(const QString& text)
{
    ui_->m_pButtonAddAlias->setEnabled(!text.isEmpty());
}

void ScreenSettingsDialog::on_m_pButtonRemoveAlias_clicked()
{
    QList<QListWidgetItem*> items = ui_->m_pListAliases->selectedItems();

    for (int i = 0; i < items.count(); i++)
        delete items[i];
}

void ScreenSettingsDialog::on_m_pListAliases_itemSelectionChanged()
{
    ui_->m_pButtonRemoveAlias->setEnabled(!ui_->m_pListAliases->selectedItems().isEmpty());
}

ScreenSettingsDialog::~ScreenSettingsDialog() = default;
