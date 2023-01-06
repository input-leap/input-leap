/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#include "inputleap/AppUtil.h"
#include <cassert>

AppUtil* AppUtil::s_instance = nullptr;

AppUtil::AppUtil() :
m_app(nullptr)
{
    s_instance = this;
}

AppUtil::~AppUtil()
{
}

void
AppUtil::adoptApp(IApp* app)
{
    app->setByeFunc(&exitAppStatic);
    m_app = app;
}

IApp&
AppUtil::app() const
{
    assert(m_app != nullptr);
    return *m_app;
}

AppUtil&
AppUtil::instance()
{
    assert(s_instance != nullptr);
    return *s_instance;
}
