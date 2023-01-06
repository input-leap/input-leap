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

#include "inputleap/ArgParser.h"
#include "inputleap/ServerArgs.h"
#include "test/mock/inputleap/MockArgParser.h"

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool
server_stubParseGenericArgs(int, const char* const*, int&)
{
    return false;
}

bool
server_stubCheckUnexpectedArgs()
{
    return false;
}

TEST(ServerArgsParsingTests, parseServerArgs_addressArg_setBarrierAddress)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
    ServerArgs serverArgs;
    const int argc = 3;
    const char* kAddressCmd[argc] = { "stub", "--address", "mock_address" };

    argParser.parseServerArgs(serverArgs, argc, kAddressCmd);

    EXPECT_EQ("mock_address", serverArgs.m_barrierAddress);
}

TEST(ServerArgsParsingTests, parseServerArgs_configArg_setConfigFile)
{
    NiceMock<MockArgParser> argParser;
    ON_CALL(argParser, parseGenericArgs(_, _, _)).WillByDefault(Invoke(server_stubParseGenericArgs));
    ON_CALL(argParser, checkUnexpectedArgs()).WillByDefault(Invoke(server_stubCheckUnexpectedArgs));
    ServerArgs serverArgs;
    const int argc = 3;
    const char* kConfigCmd[argc] = { "stub", "--config", "mock_configFile" };

    argParser.parseServerArgs(serverArgs, argc, kConfigCmd);

    EXPECT_EQ("mock_configFile", serverArgs.m_configFile);
}
