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

#include <QPixmap>
#include <QString>
#include <QList>
#include <QStringList>

#include "BaseConfig.h"

class QSettings;
class QTextStream;

class ScreenSettingsDialog;

class Screen : public BaseConfig
{
    friend QDataStream& operator<<(QDataStream& outStream, const Screen& screen);
    friend QDataStream& operator>>(QDataStream& inStream, Screen& screen);
    friend class ScreenSettingsDialog;
    friend class ScreenSetupModel;
    friend class ScreenSetupView;

    public:
        Screen();
        Screen(const QString& name);

    public:
        const QPixmap* pixmap() const { return &m_Pixmap; }
        const QString& name() const { return m_Name; }
        const QStringList& aliases() const { return m_Aliases; }

        bool isNull() const { return m_Name.isEmpty(); }
        Modifier modifier(Modifier m) const
        {
            Modifier overriddenModifier = m_Modifiers[static_cast<int>(m)];
            return overriddenModifier == Modifier::DefaultMod ? m : overriddenModifier;
        }

        const QList<Modifier>& modifiers() const { return m_Modifiers; }
        bool switchCorner(SwitchCorner c) const { return m_SwitchCorners[static_cast<int>(c)]; }
        const QList<bool>& switchCorners() const { return m_SwitchCorners; }
        int switchCornerSize() const { return m_SwitchCornerSize; }
        bool fix(Fix f) const { return m_Fixes[static_cast<int>(f)]; }
        const QList<bool>& fixes() const { return m_Fixes; }

        void loadSettings(QSettings& settings);
        void saveSettings(QSettings& settings) const;
        QTextStream& writeScreensSection(QTextStream& outStream) const;
        QTextStream& writeAliasesSection(QTextStream& outStream) const;

        bool swapped() const { return m_Swapped; }
        QString& name() { return m_Name; }
        void setName(const QString& name) { m_Name = name; }

    protected:
        void init();
        QPixmap* pixmap() { return &m_Pixmap; }

        void setPixmap(const QPixmap& pixmap) { m_Pixmap = pixmap; }
        QStringList& aliases() { return m_Aliases; }
        void setModifier(Modifier m, Modifier n) { m_Modifiers[static_cast<int>(m)] = n; }
        QList<Modifier>& modifiers() { return m_Modifiers; }
        void addAlias(const QString& alias) { m_Aliases.append(alias); }
        void setSwitchCorner(SwitchCorner c, bool on) { m_SwitchCorners[static_cast<int>(c)] = on; }
        QList<bool>& switchCorners() { return m_SwitchCorners; }
        void setSwitchCornerSize(int val) { m_SwitchCornerSize = val; }
        void setFix(Fix f, bool on) { m_Fixes[static_cast<int>(f)] = on; }
        QList<bool>& fixes() { return m_Fixes; }
        void setSwapped(bool on) { m_Swapped = on; }

    private:
        QPixmap m_Pixmap;
        QString m_Name;

        QStringList m_Aliases;
        QList<Modifier> m_Modifiers;
        QList<bool> m_SwitchCorners;
        int m_SwitchCornerSize;
        QList<bool> m_Fixes;

        bool m_Swapped;
};

QDataStream& operator<<(QDataStream& outStream, const Screen& screen);
QDataStream& operator>>(QDataStream& inStream, Screen& screen);
