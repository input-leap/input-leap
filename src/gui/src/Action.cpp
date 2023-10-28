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

#include "Action.h"

#include <QSettings>
#include <QTextStream>

const char* Action::m_ActionTypeNames[] =
{
    "keyDown", "keyUp", "keystroke",
    "switchToScreen", "toggleScreen",
    "switchInDirection", "lockCursorToScreen",
    "mouseDown", "mouseUp", "mousebutton"
};

const char* Action::m_SwitchDirectionNames[] = { "left", "right", "up", "down" };
const char* Action::m_LockCursorModeNames[] = { "toggle", "on", "off" };
const QString Action::m_commandTemplate = QStringLiteral("(%1)");


Action::Action() :
    m_KeySequence(),
    m_Type(keystroke),
    m_TypeScreenNames(),
    m_SwitchScreenName(),
    m_SwitchDirection(switchLeft),
    m_LockCursorMode(lockCursorToggle),
    m_ActiveOnRelease(false),
    m_HasScreens(false)
{
}

QString Action::text() const
{
    /* This function is used to save to config file which is for InputLeap server to
     * read. However the server config parse does not support functions with ()
     * in the end but now argument inside. If you need a function with no
     * argument, it can not have () in the end.
     */
    QString text = QString(m_ActionTypeNames[m_KeySequence.isMouseButton() ? type() + int(mouseDown) : type()]);
    switch (type())
    {
        case keyDown:
        case keyUp:
        case keystroke:
            {
                QString commandArgs = m_KeySequence.toString();
                if (!m_KeySequence.isMouseButton())
                {
                    const QStringList& screens = typeScreenNames();
                    if (haveScreens() && !screens.isEmpty())
                    {
                        QString screenList;
                        for (int i = 0; i < screens.size(); i++)
                        {
                            screenList.append(screens[i]);
                            if (i != screens.size() - 1)
                                screenList.append(QStringLiteral(":"));
                        }
                        commandArgs.append(QStringLiteral(",%1").arg(screenList));
                    }
                    else
                        commandArgs.append(QStringLiteral(",*"));
                }
                text.append(m_commandTemplate.arg(commandArgs));
            }
            break;

        case switchToScreen:
            text.append(m_commandTemplate.arg(switchScreenName()));
            break;

        case toggleScreen:
            break;

        case switchInDirection:
            text.append(m_commandTemplate.arg(m_SwitchDirectionNames[m_SwitchDirection]));
            break;

        case lockCursorToScreen:
            text.append(m_commandTemplate.arg(m_LockCursorModeNames[m_LockCursorMode]));
            break;

        default:
            Q_ASSERT(0);
            break;
    }

    return text;
}

void Action::loadSettings(QSettings& settings)
{
    m_KeySequence.loadSettings(settings);
    setType(settings.value(SETTINGS::ACTION_TYPE, keyDown).toInt());

    m_TypeScreenNames.clear();
    int numTypeScreens = settings.beginReadArray(SETTINGS::SCREEN_NAMES);
    for (int i = 0; i < numTypeScreens; i++)
    {
        settings.setArrayIndex(i);
            m_TypeScreenNames.append(settings.value(SETTINGS::SCREEN_NAME).toString());
    }
    settings.endArray();

    setSwitchScreenName(settings.value(SETTINGS::SWITCH_TO_SCREEN).toString());
    setSwitchDirection(settings.value(SETTINGS::SWITCH_DIRECTION, switchLeft).toInt());
    setLockCursorMode(settings.value(SETTINGS::LOCKTOSCREEN, lockCursorToggle).toInt());
    setActiveOnRelease(settings.value(SETTINGS::ACTIVEONRELEASE, false).toBool());
    setHaveScreens(settings.value(SETTINGS::HASSCREENS, false).toBool());
}

void Action::saveSettings(QSettings& settings) const
{
    m_KeySequence.saveSettings(settings);
    settings.setValue(SETTINGS::ACTION_TYPE, type());

    settings.beginWriteArray(SETTINGS::SCREEN_NAMES);
    for (int i = 0; i < typeScreenNames().size(); i++)
    {
        settings.setArrayIndex(i);
        settings.setValue(SETTINGS::SCREEN_NAME, typeScreenNames()[i]);
    }
    settings.endArray();

    settings.setValue(SETTINGS::SWITCH_TO_SCREEN, switchScreenName());
    settings.setValue(SETTINGS::SWITCH_DIRECTION, switchDirection());
    settings.setValue(SETTINGS::LOCKTOSCREEN, lockCursorMode());
    settings.setValue(SETTINGS::ACTIVEONRELEASE, activeOnRelease());
    settings.setValue(SETTINGS::HASSCREENS, haveScreens());
}

QTextStream& operator<<(QTextStream& outStream, const Action& action)
{
    if (action.activeOnRelease())
        outStream << ";";

    outStream << action.text();

    return outStream;
}
