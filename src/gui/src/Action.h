/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023-2024 InputLeap Developers
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

#include "KeySequence.h"

#include <QString>
#include <QStringList>
#include <QList>

class ActionDialog;
class QSettings;
class QTextStream;

namespace SettingsKeys {
    static const QString ACTION_TYPE = QStringLiteral("type");
    static const QString SCREEN_NAMES = QStringLiteral("typeScreenNames");
    static const QString SCREEN_NAME = QStringLiteral("typeScreenName");
    static const QString SWITCH_TO_SCREEN = QStringLiteral("switchScreenName");
    static const QString SWITCH_DIRECTION = QStringLiteral("switchInDirection");
    static const QString LOCKTOSCREEN = QStringLiteral("lockCursorToScreen");
    static const QString ACTIVEONRELEASE = QStringLiteral("activeOnRelease");
    static const QString HASSCREENS = QStringLiteral("hasScreens");
};

class Action
{
    public:
        enum ActionType { keyDown, keyUp, keystroke,
                          switchToScreen, toggleScreen, switchInDirection,
                          lockCursorToScreen, mouseDown, mouseUp, mousebutton };
        enum SwitchDirection { switchLeft, switchRight, switchUp, switchDown };
        enum LockCursorMode { lockCursorToggle, lockCursonOn, lockCursorOff  };

    public:
        Action();

    public:
        QString text() const;
        const KeySequence& keySequence() const { return key_sequence_; }
        void setKeySequence(const KeySequence& seq) { key_sequence_ = seq; }

        void loadSettings(QSettings& settings);
        void saveSettings(QSettings& settings) const;

        int type() const { return type_; }
        void setType(int t) { type_ = t; }

        const QStringList& typeScreenNames() const { return type_screen_names_; }
        void appendTypeScreenName(QString name) { type_screen_names_.append(name); }
        void clearTypeScreenNames() { type_screen_names_.clear(); }

        const QString& switchScreenName() const { return switch_screen_name_; }
        void setSwitchScreenName(const QString& n) { switch_screen_name_ = n; }

        int switchDirection() const { return switch_direction_; }
        void setSwitchDirection(int d) { switch_direction_ = d; }

        int lockCursorMode() const { return lock_cursor_mode_; }
        void setLockCursorMode(int m) { lock_cursor_mode_ = m; }

        bool activeOnRelease() const { return active_on_release_; }
        void setActiveOnRelease(bool b) { active_on_release_ = b; }

        bool haveScreens() const { return has_screens_; }
        void setHaveScreens(bool b) { has_screens_ = b; }

    private:
        KeySequence key_sequence_;
        int type_;
        QStringList type_screen_names_;
        QString switch_screen_name_;
        int switch_direction_;
        int lock_cursor_mode_;
        bool active_on_release_;
        bool has_screens_;

        static const char* action_type_names_[];
        static const char* switch_direction_names_[];
        static const char* lock_cursor_mode_names_[];
        static const QString command_template_;
};

QTextStream& operator<<(QTextStream& outStream, const Action& action);
