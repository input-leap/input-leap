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

#include "AboutDialog.h"

#include <QtCore>
#include <QtGui>

AboutDialog::AboutDialog(QWidget* parent, const QString& barrierApp) :
	QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	Ui::AboutDialogBase()
{
	setupUi(this);

	m_versionChecker.setApp(barrierApp);
	QString version = m_versionChecker.getVersion();
	version = version + '-' + INPUTLEAP_VERSION_STAGE;
#ifdef INPUTLEAP_REVISION
    version +=  '-';
    version += INPUTLEAP_REVISION;
#endif
	m_pLabelBarrierVersion->setText(version);

	QString buildDateString = QString::fromLocal8Bit(__DATE__).simplified();
	QDate buildDate = QLocale("en_US").toDate(buildDateString, "MMM d yyyy");
	m_pLabelBuildDate->setText(buildDate.toString(Qt::SystemLocaleLongDate));

	// change default size based on os
#if defined(Q_OS_MAC)
	QSize size(600, 380);
	setMaximumSize(size);
	setMinimumSize(size);
	resize(size);
#elif defined(Q_OS_LINUX)
	QSize size(600, 330);
	setMaximumSize(size);
	setMinimumSize(size);
	resize(size);
#endif
}
