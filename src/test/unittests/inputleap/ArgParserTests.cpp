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

#include <gtest/gtest.h>

TEST(ArgParserTests, isArg_abbreviationsArg_returnTrue)
{
    const int argc = 2;
    const char* argv[argc] = { "stub", "-t" };
    Argv a(argc, argv);

    auto result = a.shift("-t", nullptr);
    EXPECT_STREQ(result, "-t");
}

TEST(ArgParserTests, isArg_fullArg_returnTrue)
{
    const int argc = 2;
    const char* argv[argc] = { "stub", "--test" };
    Argv a(argc, argv);

    auto result = a.shift(nullptr, "--test");
    EXPECT_STREQ(result, "--test");
}

TEST(ArgParserTests, isArg_hasOptarg)
{
    const int argc = 3;
    const char* argv[argc] = { "stub", "-t", "foo" };
    Argv a(argc, argv);

     const char *optarg = NULL;
     auto result = a.shift("-t", nullptr, &optarg);

     EXPECT_STREQ(result, "-t");
     EXPECT_STREQ(optarg, "foo");
}

TEST(ArgParserTests, isArg_missingArgs_throws)
{
    const int argc = 2;
    const char* argv[argc] = { "stub", "-t" };
    Argv a(argc, argv);

    EXPECT_THROW({
             try {
                 const char *optarg = NULL;
                 auto result = a.shift("-t", nullptr, &optarg);
             } catch (XArgvParserError e) {
                 EXPECT_STREQ(e.message.c_str(), "missing argument for `-t'");
                 throw;
             }
         },
         XArgvParserError);
}

TEST(ArgParserTests, searchDoubleQuotes_doubleQuotedArg_returnTrue)
{
    std::string command = "\"stub\"";
    size_t left = 0;
    size_t right = 0;

    bool result = ArgParser::searchDoubleQuotes(command, left, right);

    EXPECT_EQ(true, result);
    EXPECT_EQ(0u, left);
    EXPECT_EQ(5u, right);
}

TEST(ArgParserTests, searchDoubleQuotes_noDoubleQuotedArg_returnfalse)
{
    std::string command = "stub";
    size_t left = 0;
    size_t right = 0;

    bool result = ArgParser::searchDoubleQuotes(command, left, right);

    EXPECT_FALSE(result);
    EXPECT_EQ(0u, left);
    EXPECT_EQ(0u, right);
}

TEST(ArgParserTests, searchDoubleQuotes_oneDoubleQuoteArg_returnfalse)
{
    std::string command = "\"stub";
    size_t left = 0;
    size_t right = 0;

    bool result = ArgParser::searchDoubleQuotes(command, left, right);

    EXPECT_FALSE(result);
    EXPECT_EQ(0u, left);
    EXPECT_EQ(0u, right);
}

TEST(ArgParserTests, splitCommandString_oneArg_returnArgv)
{
    std::string command = "stub";
    std::vector<std::string> argv;

    ArgParser::splitCommandString(command, argv);

    EXPECT_EQ(1u, argv.size());
    EXPECT_EQ("stub", argv.at(0));
}

TEST(ArgParserTests, splitCommandString_twoArgs_returnArgv)
{
    std::string command = "stub1 stub2";
    std::vector<std::string> argv;

    ArgParser::splitCommandString(command, argv);

    EXPECT_EQ(2u, argv.size());
    EXPECT_EQ("stub1", argv.at(0));
    EXPECT_EQ("stub2", argv.at(1));
}

TEST(ArgParserTests, splitCommandString_doubleQuotedArgs_returnArgv)
{
    std::string command = "\"stub1\" stub2 \"stub3\"";
    std::vector<std::string> argv;

    ArgParser::splitCommandString(command, argv);

    EXPECT_EQ(3u, argv.size());
    EXPECT_EQ("stub1", argv.at(0));
    EXPECT_EQ("stub2", argv.at(1));
    EXPECT_EQ("stub3", argv.at(2));
}

TEST(ArgParserTests, splitCommandString_spaceDoubleQuotedArgs_returnArgv)
{
    std::string command = "\"stub1\" stub2 \"stub3 space\"";
    std::vector<std::string> argv;

    ArgParser::splitCommandString(command, argv);

    EXPECT_EQ(3u, argv.size());
    EXPECT_EQ("stub1", argv.at(0));
    EXPECT_EQ("stub2", argv.at(1));
    EXPECT_EQ("stub3 space", argv.at(2));
}

TEST(ArgParserTests, getArgv_stringArray_return2DArray)
{
    std::vector<std::string> argArray;
    argArray.push_back("stub1");
    argArray.push_back("stub2");
    argArray.push_back("stub3 space");
    const char** argv = ArgParser::getArgv(argArray);

    std::string row1 = argv[0];
    std::string row2 = argv[1];
    std::string row3 = argv[2];

    EXPECT_EQ("stub1", row1);
    EXPECT_EQ("stub2", row2);
    EXPECT_EQ("stub3 space", row3);

    delete[] argv;
}

TEST(ArgParserTests, assembleCommand_stringArray_returnCommand)
{
    std::vector<std::string> argArray;
    argArray.push_back("stub1");
    argArray.push_back("stub2");
    std::string command = ArgParser::assembleCommand(argArray);

    EXPECT_EQ("stub1 stub2", command);
}

TEST(ArgParserTests, assembleCommand_ignoreSecondArg_returnCommand)
{
    std::vector<std::string> argArray;
    argArray.push_back("stub1");
    argArray.push_back("stub2");
    std::string command = ArgParser::assembleCommand(argArray, "stub2");

    EXPECT_EQ("stub1", command);
}

TEST(ArgParserTests, assembleCommand_ignoreSecondArgWithOneParameter_returnCommand)
{
    std::vector<std::string> argArray;
    argArray.push_back("stub1");
    argArray.push_back("stub2");
    argArray.push_back("stub3");
    argArray.push_back("stub4");
    std::string command = ArgParser::assembleCommand(argArray, "stub2", 1);

    EXPECT_EQ("stub1 stub4", command);
}

TEST(ArgParserTests, assembleCommand_stringArrayWithSpace_returnCommand)
{
    std::vector<std::string> argArray;
    argArray.push_back("stub1 space");
    argArray.push_back("stub2");
    argArray.push_back("stub3 space");
    std::string command = ArgParser::assembleCommand(argArray);

    EXPECT_EQ("\"stub1 space\" stub2 \"stub3 space\"", command);
}

