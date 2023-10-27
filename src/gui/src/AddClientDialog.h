/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023 InputLeap Developers
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

#ifndef ADDCLIENTDIALOG_H
#define ADDCLIENTDIALOG_H

#include <QDialog>

namespace Ui
{
    class AddClientDialog;
}

class QPushButton;
class QLabel;

enum {
    kAddClientRight,
    kAddClientLeft,
    kAddClientUp,
    kAddClientDown,
    kAddClientOther,
    kAddClientIgnore
};

class AddClientDialog : public QDialog
{
    Q_OBJECT
public:
    AddClientDialog(const QString& clientName, QWidget* parent = nullptr);
    ~AddClientDialog() = default;

    int addResult() { return m_AddResult; }
    bool ignoreAutoConfigClient() { return m_IgnoreAutoConfigClient; }

protected:
    void changeEvent(QEvent *e) override;

private slots:
    void on_m_pCheckBoxIgnoreClient_toggled(bool checked);
    void handleButtonLeft();
    void handleButtonUp();
    void handleButtonRight();
    void handleButtonDown();
    void handleButtonAdvanced();

private:
    Ui::AddClientDialog *ui = nullptr;
    QPushButton* m_pButtonLeft = nullptr;
    QPushButton* m_pButtonUp = nullptr;
    QPushButton* m_pButtonRight = nullptr;
    QPushButton* m_pButtonDown = nullptr;
    QLabel* m_pLabelCenter = nullptr;
    int m_AddResult;
    bool m_IgnoreAutoConfigClient;
};

#endif // ADDCLIENTDIALOG_H
