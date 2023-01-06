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

#if !defined(SCREENSETTINGSDIALOG__H)

#define SCREENSETTINGSDIALOG__H

#include <QDialog>

#include "ui_ScreenSettingsDialogBase.h"

class QWidget;
class QString;

class Screen;

class ScreenSettingsDialog : public QDialog, public Ui::ScreenSettingsDialogBase
{
    Q_OBJECT

    public:
        ScreenSettingsDialog(QWidget* parent, Screen* pScreen = nullptr);

    public slots:
        void accept() override;

    private slots:
        void on_m_pButtonAddAlias_clicked();
        void on_m_pButtonRemoveAlias_clicked();
        void on_m_pLineEditAlias_textChanged(const QString& text);
        void on_m_pListAliases_itemSelectionChanged();

    private:
        Screen* m_pScreen;
};

#endif
