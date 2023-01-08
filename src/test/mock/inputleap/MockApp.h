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

#define INPUTLEAP_TEST_ENV

#include "inputleap/App.h"

#include <gmock/gmock.h>

namespace inputleap {

class MockApp : public App
{
public:
    MockApp() : App(nullptr, nullptr, nullptr) { }

    MOCK_METHOD0(help, void());
    MOCK_METHOD0(loadConfig, void());
    MOCK_METHOD1(loadConfig, bool(const std::string&));
    MOCK_CONST_METHOD0(daemonInfo, const char*());
    MOCK_CONST_METHOD0(daemonName, const char*());
    MOCK_METHOD2(parseArgs, void(int, const char* const*));
    MOCK_METHOD0(version, void());
    MOCK_METHOD2(standardStartup, int(int, char**));
    MOCK_METHOD4(runInner, int(int, char**, ILogOutputter*, StartupFunc));
    MOCK_METHOD0(startNode, void());
    MOCK_METHOD0(mainLoop, int());
    MOCK_METHOD2(foregroundStartup, int(int, char**));
    MOCK_METHOD0(createScreen, inputleap::Screen*());
};

} // namespace inputleap
