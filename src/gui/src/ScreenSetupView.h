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

#pragma once

#include <QTableView>
#include <QFlags>

#include "Screen.h"

class QWidget;
class QMouseEvent;
class QResizeEvent;
class QDragEnterEvent;
class ScreenSetupModel;

class ScreenSetupView : public QTableView
{
    Q_OBJECT

    public:
        ScreenSetupView(QWidget* parent);

    public:
        void setModel(QAbstractItemModel* model) override;
        ScreenSetupModel* model() const;

    protected:
        void mouseDoubleClickEvent(QMouseEvent*) override;
        void keyPressEvent(QKeyEvent*) override;
        void setTableSize();
        void resizeEvent(QResizeEvent*) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragMoveEvent(QDragMoveEvent* event) override;
        void startDrag(Qt::DropActions supportedActions) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        void initViewItemOption(QStyleOptionViewItem *option) const override;
#else
        QStyleOptionViewItem viewOptions() const override;
#endif
        void scrollTo(const QModelIndex&, ScrollHint) override {}
    private:
        void enter(const QModelIndex&);
        void remove(const QModelIndex&);
};
