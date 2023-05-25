/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 */

#include "AppLocale.h"

#include <QResource>
#include <QXmlStreamReader>
#include <QDebug>

AppLocale::AppLocale()
{
    loadLanguages();
}

void AppLocale::loadLanguages()
{
    QResource resource(":/res/lang/Languages.xml");
    QByteArray bytes(reinterpret_cast<const char*>(resource.data()), resource.size());
    QXmlStreamReader xml(bytes);

    while (!xml.atEnd())
    {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (xml.hasError())
        {
            qCritical() << xml.errorString();
            throw std::exception();
        }

        if (xml.name() == QLatin1String("language") && token == QXmlStreamReader::StartElement)
        {
            QXmlStreamAttributes attributes = xml.attributes();
            addLanguage(
                attributes.value("ietfCode").toString(),
                attributes.value("name").toString());
        }
    }
}

void AppLocale::addLanguage(const QString& ietfCode, const QString& name)
{
    m_Languages.push_back(AppLocale::Language(ietfCode, name));
}

void AppLocale::fillLanguageComboBox(QComboBox* comboBox)
{
    comboBox->blockSignals(true);
    QVector<AppLocale::Language>::iterator it;
    for (it = m_Languages.begin(); it != m_Languages.end(); ++it)
    {
        comboBox->addItem((*it).m_Name, (*it).m_IetfCode);
    }
    comboBox->blockSignals(false);
}
