/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023 InputLeap Developers
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

#include "AboutDialog.h"
#include "ui_AboutDialog.h"

#include "common/Version.h"

AboutDialog::AboutDialog(QWidget* parent, const QString& app_name) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    QString version = QStringLiteral("%1-%2").arg(kVersion, INPUTLEAP_VERSION_STAGE);
#ifdef INPUTLEAP_REVISION
    version.append(QStringLiteral("-%1").arg(INPUTLEAP_REVISION));
#endif
    ui->m_pLabelAppVersion->setText(version);

    //logoHeight is the real pixel height of the resource.
    const int logoHeight = 45;
    //set the app logo scale to width of the dialog
    //the scale can range from 1 - 3 x
    ui->m_pLabelAppLogo->setPixmap(QPixmap(":/res/image/about.png").scaled(QSize(sizeHint().width(), std::max(logoHeight, std::min(135, (sizeHint().width()/163) * logoHeight))) , Qt::KeepAspectRatio, Qt::SmoothTransformation));
    setFixedSize( sizeHint() );
}
