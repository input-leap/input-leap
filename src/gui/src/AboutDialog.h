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

#if !defined(ABOUTDIALOG__H)

#define ABOUTDIALOG__H

#include <QDialog>
#include "VersionChecker.h"

#include "ui_AboutDialogBase.h"

class QWidget;
class QString;

class AboutDialog : public QDialog, public Ui::AboutDialogBase
{
    Q_OBJECT

    public:
        AboutDialog(QWidget* parent, const QString& barrierApp = QString());

    private:
        VersionChecker m_versionChecker;
};

#endif
