/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023 Input Leap Devs
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

#if !defined(ACTIONDIALOG_H)

#define ACTIONDIALOG_H

#include <QDialog>
#include "KeySequenceWidget.h"

namespace Ui
{
    class ActionDialog;
}

class Hotkey;
class Action;
class QRadioButton;
class QButtonGroup;
class ServerConfig;

class ActionDialog : public QDialog
{
    Q_OBJECT
    public:
        ActionDialog(QWidget* parent, ServerConfig& config, Hotkey& hotkey, Action& action);

    protected slots:
        void accept() override;
        void on_m_pKeySequenceWidgetHotkey_keySequenceChanged();

    protected:
        const KeySequenceWidget* sequenceWidget() const;
        const ServerConfig& serverConfig() const { return m_ServerConfig; }

    private:
        Ui::ActionDialog *ui = nullptr;
        const ServerConfig& m_ServerConfig;
        Hotkey& m_Hotkey;
        Action& m_Action;

        QButtonGroup* m_pButtonGroupType;
};
#endif
