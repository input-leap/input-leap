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

#include "Action.h"

#include <QSettings>
#include <QTextStream>

const char* Action::action_type_names_[] =
{
    "keyDown", "keyUp", "keystroke",
    "switchToScreen", "toggleScreen",
    "switchInDirection", "lockCursorToScreen",
    "mouseDown", "mouseUp", "mousebutton"
};

const char* Action::switch_direction_names_[] = { "left", "right", "up", "down" };
const char* Action::lock_cursor_mode_names_[] = { "toggle", "on", "off" };

Action::Action() :
    key_sequence_(),
    type_(keystroke),
    type_screen_names_(),
    switch_screen_name_(),
    switch_direction_(switchLeft),
    lock_cursor_mode_(lockCursorToggle),
    active_on_release_(false),
    has_screens_(false)
{
}

QString Action::text() const
{
    /* This function is used to save to config file which is for InputLeap server to
     * read. However the server config parse does not support functions with ()
     * in the end but now argument inside. If you need a function with no
     * argument, it can not have () in the end.
     */
    QString text = QString(action_type_names_[key_sequence_.isMouseButton() ?
                                             type() + int(mouseDown) : type()]);

    switch (type())
    {
        case keyDown:
        case keyUp:
        case keystroke:
            {
                text += "(";
                text += key_sequence_.toString();

                if (!key_sequence_.isMouseButton())
                {
                    const QStringList& screens = typeScreenNames();
                    if (haveScreens() && !screens.isEmpty())
                    {
                        text += ",";

                        for (int i = 0; i < screens.size(); i++)
                        {
                            text += screens[i];
                            if (i != screens.size() - 1)
                                text += ":";
                        }
                    }
                    else
                        text += ",*";
                }
                text += ")";
            }
            break;

        case switchToScreen:
            text += "(";
            text += switchScreenName();
            text += ")";
            break;

        case toggleScreen:
            break;

        case switchInDirection:
            text += "(";
            text += switch_direction_names_[switch_direction_];
            text += ")";
            break;

        case lockCursorToScreen:
            text += "(";
            text += lock_cursor_mode_names_[lock_cursor_mode_];
            text += ")";
            break;

        default:
            Q_ASSERT(0);
            break;
    }


    return text;
}

void Action::loadSettings(QSettings& settings)
{
    key_sequence_.loadSettings(settings);
    setType(settings.value("type", keyDown).toInt());

    type_screen_names_.clear();
    int numTypeScreens = settings.beginReadArray("typeScreenNames");
    for (int i = 0; i < numTypeScreens; i++)
    {
        settings.setArrayIndex(i);
        type_screen_names_.append(settings.value(SettingsKeys::SCREEN_NAME).toString());
    }
    settings.endArray();

    setSwitchScreenName(settings.value(SettingsKeys::SWITCH_TO_SCREEN).toString());
    setSwitchDirection(settings.value(SettingsKeys::SWITCH_DIRECTION, switchLeft).toInt());
    setLockCursorMode(settings.value(SettingsKeys::LOCKTOSCREEN, lockCursorToggle).toInt());
    setActiveOnRelease(settings.value(SettingsKeys::ACTIVEONRELEASE, false).toBool());
    setHaveScreens(settings.value(SettingsKeys::HASSCREENS, false).toBool());
}

void Action::saveSettings(QSettings& settings) const
{
    key_sequence_.saveSettings(settings);
    settings.setValue(SettingsKeys::ACTION_TYPE, type());

    settings.beginWriteArray(SettingsKeys::SCREEN_NAMES);
    for (int i = 0; i < typeScreenNames().size(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue(SettingsKeys::SCREEN_NAME, typeScreenNames()[i]);
    }
    settings.endArray();

    settings.setValue(SettingsKeys::SWITCH_TO_SCREEN, switchScreenName());
    settings.setValue(SettingsKeys::SWITCH_DIRECTION, switchDirection());
    settings.setValue(SettingsKeys::LOCKTOSCREEN, lockCursorMode());
    settings.setValue(SettingsKeys::ACTIVEONRELEASE, activeOnRelease());
    settings.setValue(SettingsKeys::HASSCREENS, haveScreens());
}

QTextStream& operator<<(QTextStream& outStream, const Action& action)
{
    if (action.activeOnRelease())
        outStream << ";";

    outStream << action.text();

    return outStream;
}
