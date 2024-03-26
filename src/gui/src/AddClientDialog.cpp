/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "AddClientDialog.h"
#include "ui_AddClientDialog.h"

#include <QPushButton>
#include <QLabel>

AddClientDialog::AddClientDialog(const QString& clientName, QWidget* parent) :
    QDialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
    ui_{std::make_unique<Ui::AddClientDialog>()},
    m_AddResult(kAddClientIgnore),
    m_IgnoreAutoConfigClient(false)
{
    ui_->setupUi(this);

    ui_->m_pLabelHead->setText("A client wants to connect. "
                    "Please choose a location for " + clientName + ".");

    QIcon icon(":res/icons/64x64/video-display.png");
    QSize IconSize(32,32);

    m_pButtonLeft = new QPushButton(this);
    m_pButtonLeft->setIcon(icon);
    m_pButtonLeft->setIconSize(IconSize);
    ui_->gridLayout->addWidget(m_pButtonLeft, 2, 0, 1, 1, Qt::AlignCenter);
    connect(m_pButtonLeft, &QPushButton::clicked, this, &AddClientDialog::handleButtonLeft);

    m_pButtonUp = new QPushButton(this);
    m_pButtonUp->setIcon(icon);
    m_pButtonUp->setIconSize(IconSize);
    ui_->gridLayout->addWidget(m_pButtonUp, 1, 1, 1, 1, Qt::AlignCenter);
    connect(m_pButtonUp, &QPushButton::clicked, this, &AddClientDialog::handleButtonUp);

    m_pButtonRight = new QPushButton(this);
    m_pButtonRight->setIcon(icon);
    m_pButtonRight->setIconSize(IconSize);
    ui_->gridLayout->addWidget(m_pButtonRight, 2, 2, 1, 1, Qt::AlignCenter);
    connect(m_pButtonRight, &QPushButton::clicked, this, &AddClientDialog::handleButtonRight);

    m_pButtonDown = new QPushButton(this);
    m_pButtonDown->setIcon(icon);
    m_pButtonDown->setIconSize(IconSize);
    ui_->gridLayout->addWidget(m_pButtonDown, 3, 1, 1, 1, Qt::AlignCenter);
    connect(m_pButtonDown, &QPushButton::clicked, this, &AddClientDialog::handleButtonDown);

    m_pLabelCenter = new QLabel(this);
    m_pLabelCenter->setPixmap(QPixmap(":res/icons/64x64/video-display.png"));
    ui_->gridLayout->addWidget(m_pLabelCenter, 2, 1, 1, 1, Qt::AlignCenter);

#if defined(Q_OS_MAC)
    ui_->m_pDialogButtonBox->setLayoutDirection(Qt::RightToLeft);
#endif

    QPushButton* advanced = ui_->m_pDialogButtonBox->addButton("Advanced",
                                                    QDialogButtonBox::HelpRole);
    connect(advanced, &QPushButton::clicked, this, &AddClientDialog::handleButtonAdvanced);
}

AddClientDialog::~AddClientDialog()
{
    delete m_pButtonUp;
    delete m_pButtonDown;
    delete m_pButtonLeft;
    delete m_pButtonRight;
    delete m_pLabelCenter;
}

void AddClientDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui_->retranslateUi(this);
        break;
    default:
        break;
    }
}

void AddClientDialog::handleButtonLeft()
{
    m_AddResult = kAddClientLeft;
    close();
}

void AddClientDialog::handleButtonUp()
{
    m_AddResult = kAddClientUp;
    close();
}

void AddClientDialog::handleButtonRight()
{
    m_AddResult = kAddClientRight;
    close();
}

void AddClientDialog::handleButtonDown()
{
    m_AddResult = kAddClientDown;
    close();
}

void AddClientDialog::handleButtonAdvanced()
{
    m_AddResult = kAddClientOther;
    close();
}

void AddClientDialog::on_m_pCheckBoxIgnoreClient_toggled(bool checked)
{
    m_IgnoreAutoConfigClient = checked;
}
