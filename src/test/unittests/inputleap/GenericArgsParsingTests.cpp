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

#include "inputleap/ArgParser.h"
#include "inputleap/ArgsBase.h"
#include "test/mock/inputleap/MockApp.h"

#include <gtest/gtest.h>

using namespace inputleap;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;

bool g_helpShowed = false;
bool g_versionShowed = false;

void
showMockHelp()
{
    g_helpShowed = true;
}

void
showMockVersion()
{
    g_versionShowed = true;
}

TEST(GenericArgsParsingTests, parseGenericArgs_logLevelCmd_setLogLevel)
{
    const int argc = 3;
    const char* kLogLevelCmd[argc] = { "stub", "--debug", "DEBUG" };
    Argv a(argc, kLogLevelCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    std::string logFilter = argsBase.m_logFilter;

    EXPECT_EQ("DEBUG", logFilter);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_logFileCmd_saveLogFilename)
{
    const int argc = 3;
    const char* kLogFileCmd[argc] = { "stub", "--log", "mock_filename" };
    Argv a(argc, kLogFileCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    std::string logFile = argsBase.m_logFile;

    EXPECT_EQ("mock_filename", logFile);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_logFileCmdWithSpace_saveLogFilename)
{
    const int argc = 3;
    const char* kLogFileCmdWithSpace[argc] = { "stub", "--log", "mo ck_filename" };
    Argv a(argc, kLogFileCmdWithSpace);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    std::string logFile = argsBase.m_logFile;

    EXPECT_EQ("mo ck_filename", logFile);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_noDeamonCmd_daemonFalse)
{
    const int argc = 2;
    const char* kNoDaemonCmd[argc] = { "stub", "-f" };
    Argv a(argc, kNoDaemonCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_FALSE(argsBase.m_daemon);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_deamonCmd_daemonTrue)
{
    const int argc = 2;
    const char* kDaemonCmd[argc] = { "stub", "--daemon" };
    Argv a(argc, kDaemonCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_daemon);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_nameCmd_saveName)
{
    const int argc = 3;
    const char* kNameCmd[argc] = { "stub", "--name", "mock" };
    Argv a(argc, kNameCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ("mock", argsBase.m_name);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_noRestartCmd_restartFalse)
{
    int i = 1;
    const int argc = 2;
    const char* kNoRestartCmd[argc] = { "stub", "--no-restart" };
    Argv a(argc, kNoRestartCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_FALSE(argsBase.m_restartable);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_restartCmd_restartTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kRestartCmd[argc] = { "stub", "--restart" };
    Argv a(argc, kRestartCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_restartable);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_backendCmd_backendTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kBackendCmd[argc] = { "stub", "-z" };
    Argv a(argc, kBackendCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_backend);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_noHookCmd_noHookTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kNoHookCmd[argc] = { "stub", "--no-hooks" };
    Argv a(argc, kNoHookCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_noHooks);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_helpCmd_showHelp)
{
    g_helpShowed = false;
    int i = 1;
    const int argc = 2;
    const char* kHelpCmd[argc] = { "stub", "--help" };
    Argv a(argc, kHelpCmd);

    NiceMock<MockApp> app;
    ArgParser argParser(&app);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);
    ON_CALL(app, help()).WillByDefault(Invoke(showMockHelp));

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, g_helpShowed);
    EXPECT_EQ(a.size(), 0); // all args consumed
}


TEST(GenericArgsParsingTests, parseGenericArgs_versionCmd_showVersion)
{
    g_versionShowed = false;
    int i = 1;
    const int argc = 2;
    const char* kVersionCmd[argc] = { "stub", "--version" };
    Argv a(argc, kVersionCmd);

    NiceMock<MockApp> app;
    ArgParser argParser(&app);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);
    ON_CALL(app, version()).WillByDefault(Invoke(showMockVersion));

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, g_versionShowed);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_noTrayCmd_disableTrayTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kNoTrayCmd[argc] = { "stub", "--no-tray" };
    Argv a(argc, kNoTrayCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_disableTray);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

TEST(GenericArgsParsingTests, parseGenericArgs_ipcCmd_enableIpcTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kIpcCmd[argc] = { "stub", "--ipc" };
    Argv a(argc, kIpcCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_enableIpc);
    EXPECT_EQ(a.size(), 0); // all args consumed
}

#ifndef  WINAPI_XWINDOWS
TEST(GenericArgsParsingTests, parseGenericArgs_dragDropCmdOnNonLinux_enableDragDropTrue)
{
    int i = 1;
    const int argc = 2;
    const char* kDragDropCmd[argc] = { "stub", "--enable-drag-drop" };
    Argv a(argc, kDragDropCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_EQ(true, argsBase.m_enableDragDrop);
    EXPECT_EQ(a.size(), 0); // all args consumed
}
#endif

#ifdef  WINAPI_XWINDOWS
TEST(GenericArgsParsingTests, parseGenericArgs_dragDropCmdOnLinux_enableDragDropFalse)
{
    int i = 1;
    const int argc = 2;
    const char* kDragDropCmd[argc] = { "stub", "--enable-drag-drop" };
    Argv a(argc, kDragDropCmd);

    ArgParser argParser(nullptr);
    ArgsBase argsBase;
    argParser.setArgsBase(argsBase);

    argParser.parseGenericArgs(a);

    EXPECT_FALSE(argsBase.m_enableDragDrop);
    EXPECT_EQ(a.size(), 0); // all args consumed
}
#endif
