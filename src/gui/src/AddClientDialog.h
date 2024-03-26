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

#pragma once

#include <QDialog>
#include <memory>

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
namespace Ui
{
    class AddClientDialog;
}

class AddClientDialog : public QDialog
{
    Q_OBJECT
public:
    AddClientDialog(const QString& clientName, QWidget* parent = nullptr);
    ~AddClientDialog() override;

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
    std::unique_ptr<Ui::AddClientDialog> ui_;
    QPushButton* m_pButtonLeft;
    QPushButton* m_pButtonUp;
    QPushButton* m_pButtonRight;
    QPushButton* m_pButtonDown;
    QLabel* m_pLabelCenter;
    int m_AddResult;
    bool m_IgnoreAutoConfigClient;
};
