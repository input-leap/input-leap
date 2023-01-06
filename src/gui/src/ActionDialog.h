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

#if !defined(ACTIONDIALOG_H)

#define ACTIONDIALOG_H

#include <QDialog>

#include "ui_ActionDialogBase.h"

class Hotkey;
class Action;
class QRadioButton;
class QButtonGroup;
class ServerConfig;

class ActionDialog : public QDialog, public Ui::ActionDialogBase
{
    Q_OBJECT

    public:
        ActionDialog(QWidget* parent, ServerConfig& config, Hotkey& hotkey, Action& action);

    protected slots:
        void accept() override;
        void on_m_pKeySequenceWidgetHotkey_keySequenceChanged();

    protected:
        const KeySequenceWidget* sequenceWidget() const { return m_pKeySequenceWidgetHotkey; }
        const ServerConfig& serverConfig() const { return m_ServerConfig; }

    private:
        const ServerConfig& m_ServerConfig;
        Hotkey& m_Hotkey;
        Action& m_Action;

        QButtonGroup* m_pButtonGroupType;
};

#endif
