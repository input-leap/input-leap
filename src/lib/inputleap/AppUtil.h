/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "inputleap/IAppUtil.h"
#include "inputleap/XBarrier.h"

class AppUtil : public IAppUtil {
public:
    AppUtil();
    ~AppUtil() override;

    void adoptApp(IApp* app) override;
    IApp& app() const override;
    virtual void exitApp(int code) { throw XExitApp(code); }

    static AppUtil& instance();
    static void exitAppStatic(int code) { instance().exitApp(code); }
    void beforeAppExit() override {}

private:
    IApp* m_app;
    static AppUtil* s_instance;
};
