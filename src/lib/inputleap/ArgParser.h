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

#include "common/stdvector.h"

#include <string>
#include <deque>

class ServerArgs;
class ClientArgs;
class ArgsBase;
class App;

// Thrown whenever the option parsing fails.
class XArgvParserError {
public:
    XArgvParserError(const char *fmt, ...);
    std::string message;
};

// Wrapper class around int argc, char **argv to make it easier to
// iterate through arguments. This is a single-use class, once
// Argv::empty() returns true, it needs to be discarded and re-initialized.
class Argv {
public:
    Argv(int argc, const char* const* argv);

    // Return the next argument (excluding argv[0]) and remove it from the list
    const char* shift();

    // Return the next argument (excluding argv[0]) and remove it from the list,
    // if either name1 or name2 match the argument. If optarg is not null, it set
    // to the subsequent argument which is also removed from the list.
    const char* shift(const char *name1, const char*name2 = nullptr, const char** optarg = nullptr);

    // Return the next argument (excluding argv[0]) but do not remove it from the list
    const char* peek() { return m_argv.front(); }

    // Return true if the given name is in the argument list
    bool contains(const char *name);

    // True if no more arguments are available
    bool empty() { return m_argv.empty(); }

    // Returns the size of the argument list
    int size() { return m_argv.size(); }

    const std::string& exename() { return m_exename; };

private:
    std::deque<const char*> m_argv;
    std::string m_exename;
};

class ArgParser {

public:
    ArgParser(App* app);

    bool parseServerArgs(ServerArgs& args, int argc, const char* const* argv);
    bool parseClientArgs(ClientArgs& args, int argc, const char* const* argv);
    bool parsePlatformArg(ArgsBase& argsBase, Argv& argv);
    bool parseGenericArgs(Argv& argv);
    void setArgsBase(ArgsBase& argsBase) { m_argsBase = &argsBase; }

    static void splitCommandString(std::string& command, std::vector<std::string>& argv);
    static bool searchDoubleQuotes(std::string& command, size_t& left,
                            size_t& right, size_t startPos = 0);
    static void removeDoubleQuotes(std::string& arg);
    static const char** getArgv(std::vector<std::string>& argsArray);
    static std::string assembleCommand(std::vector<std::string>& argsArray,
                                       std::string ignoreArg = "", int parametersRequired = 0);

    static std::string parse_exename(const char* arg);

private:
    void updateCommonArgs(Argv &argv);

    static ArgsBase& argsBase() { return *m_argsBase; }

    bool parseMSWindowsArg(ArgsBase& argsBase, Argv& argv);
    bool parseCarbonArg(ArgsBase& argsBase, Argv& argv);
    bool parseXWindowsArg(ArgsBase& argsBase, Argv& argv);

private:
    App* m_app;

    static ArgsBase* m_argsBase;
};
