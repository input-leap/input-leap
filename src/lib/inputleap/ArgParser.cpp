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

#include "inputleap/StreamChunker.h"
#include "inputleap/App.h"
#include "inputleap/ServerArgs.h"
#include "inputleap/ClientArgs.h"
#include "inputleap/ArgsBase.h"
#include "base/Log.h"
#include "base/String.h"
#include "io/filesystem.h"

#ifdef WINAPI_MSWINDOWS
#include <VersionHelpers.h>
#endif

namespace inputleap {

XArgvParserError::XArgvParserError(const char *fmt, ...) :
    message("Unknown reason")
{
    char buf[1024] = {0}; // we accept truncation of too long messages
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);

    message = std::string(buf);
}

Argv::Argv(int argc, const char* const* argv) :
    // FIXME: we assume UTF-8 encoding, but on Windows this is not correct
    m_exename(inputleap::fs::u8path(argv[0]).filename().u8string())
{
    for (int i = 1; i < argc; i++)
        m_argv.push_back(argv[i]);
}

const char*
Argv::shift()
{
    if (m_argv.empty())
        return nullptr;

    auto a = m_argv.front();
    m_argv.pop_front();
    return a;
}

const char*
Argv::shift(const char *name1, const char* name2, const char **optarg)
{
    if (m_argv.empty())
        return nullptr;

    auto a = m_argv.front();
    if ((name1 != nullptr && strcmp(a, name1) == 0) ||
        (name2 != nullptr && strcmp(a, name2) == 0)) {
        if (optarg != nullptr && m_argv.size() <= 1) {
            throw XArgvParserError("missing argument for `%s'", a);
        }
        m_argv.pop_front();
        if (optarg != nullptr) {
            *optarg = m_argv.front();
            m_argv.pop_front();
        }
        return a;
    }
    return nullptr;
}

bool
Argv::contains(const char *name)
{
    for (auto it = m_argv.begin(); it != m_argv.end(); it++) {
        if (strcmp(*it, name) == 0)
            return true;
    }
    return false;
}


ArgsBase* ArgParser::m_argsBase = nullptr;

ArgParser::ArgParser(App* app) :
    m_app(app)
{
}

bool
ArgParser::parseServerArgs(ServerArgs& args, int argc, const char* const* argv)
{
    setArgsBase(args);

    Argv a(argc, argv);
    updateCommonArgs(a);

    try {
        while (!a.empty()) {
            const char *optarg = NULL;

            if (parsePlatformArg(args, a)) {
                continue;
            }
            else if (parseGenericArgs(a)) {
                continue;
            }
            else if (a.shift("-a", "--address", &optarg)) {
                // save listen address
                args.network_address = optarg;
            }
            else if (a.shift("-c", "--config", &optarg)) {
                // save configuration file path
                args.m_configFile = optarg;
            }
            else if (a.shift("--screen-change-script", nullptr, &optarg)) {
                // save screen change script path
                args.m_screenChangeScript = optarg;
            }
            else if (a.shift("--disable-client-cert-checking")) {
                args.check_client_certificates = false;
            } else {
                throw XArgvParserError("unrecognized option `%s'", a.peek());
            }
        }
    } catch (XArgvParserError e) {
        LOG((CLOG_PRINT "%s: %s" BYE, a.exename().c_str(), e.message.c_str(), a.exename().c_str()));
        return false;
    }

    return true;
}

bool
ArgParser::parseClientArgs(ClientArgs& args, int argc, const char* const* argv)
{
    setArgsBase(args);

    Argv a(argc, argv);
    updateCommonArgs(a);

    try {
        if (a.empty())
            throw XArgvParserError("a server address or name is required");

        while (!a.empty()) {
            const char *optarg = NULL;

            if (parsePlatformArg(args, a)) {
                continue;
            }
            else if (parseGenericArgs(a)) {
                continue;
            }
            else if (a.shift("--yscroll", nullptr, &optarg)) {
                // define scroll
                args.m_yscroll = atoi(optarg);
            }
            else if (a.size() == 1) {
                args.network_address = a.shift();
                return true;
            } else {
                throw XArgvParserError("unrecognized option `%s'", a.peek());
            }
        }

        if (args.network_address.empty())
            throw XArgvParserError("a server address or name is required");

    } catch (XArgvParserError e) {
        LOG((CLOG_PRINT "%s: %s" BYE, a.exename().c_str(), e.message.c_str(), a.exename().c_str()));
        return false;
    }

    if (args.m_shouldExit)
        return true;

    return true;
}

#if WINAPI_MSWINDOWS
bool
ArgParser::parseMSWindowsArg(ArgsBase& argsBase, Argv& argv)
{
    if (argv.shift("--service")) {
        LOG((CLOG_WARN "obsolete argument --service, use barrierd instead."));
        argsBase.m_shouldExit = true;
    }
    else if (argv.shift("--exit-pause")) {
        argsBase.m_pauseOnExit = true;
    }
    else if (argv.shift("--stop-on-desk-switch")) {
        argsBase.m_stopOnDeskSwitch = true;
    }
    else {
        // option not supported here
        return false;
    }

    return true;
}
#endif

#if WINAPI_CARBON
bool
ArgParser::parseCarbonArg(ArgsBase& argsBase, Argv& argv)
{
    // no options for carbon
    return false;
}
#endif

#if WINAPI_XWINDOWS
bool
ArgParser::parseXWindowsArg(ArgsBase& argsBase, Argv& argv)
{
    const char* optarg = NULL;

    if (argv.shift("-display", "--display", &optarg)) {
        // use alternative display
        argsBase.m_display = optarg;
    }
    else if (argv.shift("--no-xinitthreads")) {
        LOG((CLOG_NOTE "--no-xinitthreads is deprecated"));
    } else {
        // option not supported here
        return false;
    }

    return true;
}
#endif

bool
ArgParser::parsePlatformArg(ArgsBase& argsBase, Argv& argv)
{
#if WINAPI_MSWINDOWS
    return parseMSWindowsArg(argsBase, argv);
#endif
#if WINAPI_CARBON
    return parseCarbonArg(argsBase, argv);
#endif
#if WINAPI_XWINDOWS
    return parseXWindowsArg(argsBase, argv);
#endif
    return false;
}

bool
ArgParser::parseGenericArgs(Argv& argv)
{
    const char *optarg = NULL;

    if (argv.shift("-d", "--debug", &optarg)) {
        // change logging level
        argsBase().m_logFilter = optarg;
    }
    else if (argv.shift("-l", "--log", &optarg)) {
        argsBase().m_logFile = optarg;
    }
    else if (argv.shift("-f", "--no-daemon")) {
        // not a daemon
        argsBase().m_daemon = false;
    }
    else if (argv.shift("--daemon")) {
#if SYSAPI_WIN32
        // suggest that user installs as a windows service. when launched as
        // service, process should automatically detect that it should run in
        // daemon mode.
        throw XArgvParserError(
            "the --daemon argument is not supported on windows. "
            "instead, install %s as a service (--service install)",
            argv.exename().c_str());
#else
        // daemonize
        argsBase().m_daemon = true;
#endif
    }
    else if (argv.shift("-n", "--name", &optarg)) {
        // save screen name
        argsBase().m_name = optarg;
    }
    else if (argv.shift("-1", "--no-restart")) {
        // don't try to restart
        argsBase().m_restartable = false;
    }
    else if (argv.shift("--restart")) {
        // try to restart
        argsBase().m_restartable = true;
    }
    else if (argv.shift("-z")) {
        argsBase().m_backend = true;
    }
    else if (argv.shift("--no-hooks")) {
        argsBase().m_noHooks = true;
    }
    else if (argv.shift("-h", "--help")) {
        if (m_app) {
            m_app->help();
        }
        argsBase().m_shouldExit = true;
    }
    else if (argv.shift("--version")) {
        if (m_app) {
            m_app->version();
        }
        argsBase().m_shouldExit = true;
    }
    else if (argv.shift("--no-tray")) {
        argsBase().m_disableTray = true;
    }
    else if (argv.shift("--ipc")) {
        argsBase().m_enableIpc = true;
    }
    else if (argv.shift("--server")) {
        // HACK: stop error happening when using portable app.
        // FIXME: there is no portable InputLeap
    }
    else if (argv.shift("--client")) {
        // HACK: stop error happening when using portable app.
        // FIXME: there is no portable InputLeap.
    }
    else if (argv.shift("--enable-drag-drop")) {
        bool useDragDrop = true;

#ifdef WINAPI_XWINDOWS

        useDragDrop = false;
        LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported on linux."));

#endif

        if (useDragDrop) {
            argsBase().m_enableDragDrop = true;
        }
    }
    else if (argv.shift("--drop-dir")) {
        argsBase().m_dropTarget = argv.shift();
    }
    else if (argv.shift("--enable-crypto")) {
        LOG((CLOG_INFO "--enable-crypto is used by default. The option is deprecated."));
    }
    else if (argv.shift("--disable-crypto")) {
        argsBase().m_enableCrypto = false;
    }
    else if (argv.shift("--profile-dir", nullptr, &optarg)) {
        argsBase().m_profileDirectory = inputleap::fs::u8path(optarg);
    }
    else if (argv.shift("--plugin-dir", nullptr, &optarg)) {
        argsBase().m_pluginDirectory = inputleap::fs::u8path(optarg);
    }
    else {
        // option not supported here
        return false;
    }

    return true;
}

void ArgParser::splitCommandString(std::string& command, std::vector<std::string>& argv)
{
    if (command.empty()) {
        return ;
    }

    size_t leftDoubleQuote = 0;
    size_t rightDoubleQuote = 0;
    searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote);

    size_t startPos = 0;
    size_t space = command.find(" ", startPos);

    while (space != std::string::npos) {
        bool ignoreThisSpace = false;

        // check if the space is between two double quotes
        if (space > leftDoubleQuote && space < rightDoubleQuote) {
            ignoreThisSpace = true;
        }
        else if (space > rightDoubleQuote) {
            searchDoubleQuotes(command, leftDoubleQuote, rightDoubleQuote, rightDoubleQuote + 1);
        }

        if (!ignoreThisSpace) {
            std::string subString = command.substr(startPos, space - startPos);

            removeDoubleQuotes(subString);
            argv.push_back(subString);
        }

        // find next space
        if (ignoreThisSpace) {
            space = command.find(" ", rightDoubleQuote + 1);
        }
        else {
            startPos = space + 1;
            space = command.find(" ", startPos);
        }
    }

    std::string subString = command.substr(startPos, command.size());
    removeDoubleQuotes(subString);
    argv.push_back(subString);
}

bool ArgParser::searchDoubleQuotes(std::string& command, size_t& left, size_t& right, size_t startPos)
{
    bool result = false;
    left = std::string::npos;
    right = std::string::npos;

    left = command.find("\"", startPos);
    if (left != std::string::npos) {
        right = command.find("\"", left + 1);
        if (right != std::string::npos) {
            result = true;
        }
    }

    if (!result) {
        left = 0;
        right = 0;
    }

    return result;
}

void ArgParser::removeDoubleQuotes(std::string& arg)
{
    // if string is surrounded by double quotes, remove them
    if (arg[0] == '\"' &&
        arg[arg.size() - 1] == '\"') {
        arg = arg.substr(1, arg.size() - 2);
    }
}

const char** ArgParser::getArgv(std::vector<std::string>& argsArray)
{
    size_t argc = argsArray.size();

    // caller is responsible for deleting the outer array only
    // we use the c string pointers from argsArray and assign
    // them to the inner array. So caller only need to use
    // delete[] to delete the outer array
    const char** argv = new const char*[argc];

    for (size_t i = 0; i < argc; i++) {
        argv[i] = argsArray[i].c_str();
    }

    return argv;
}

std::string ArgParser::assembleCommand(std::vector<std::string>& argsArray,
                                       std::string ignoreArg, int parametersRequired)
{
    std::string result;

    for (std::vector<std::string>::iterator it = argsArray.begin(); it != argsArray.end(); ++it) {
        if (it->compare(ignoreArg) == 0) {
            it = it + parametersRequired;
            continue;
        }

        // if there is a space in this arg, use double quotes surround it
        if ((*it).find(" ") != std::string::npos) {
            (*it).insert(0, "\"");
            (*it).push_back('\"');
        }

        result.append(*it);
        // add space to saperate args
        result.append(" ");
    }

    if (!result.empty()) {
        // remove the tail space
        result = result.substr(0, result.size() - 1);
    }

    return result;
}

void
ArgParser::updateCommonArgs(Argv &argv)
{
    argsBase().m_name = ARCH->getHostName();
    argsBase().m_exename = std::string(argv.exename());
}

std::string ArgParser::parse_exename(const char* arg)
{
    // FIXME: we assume UTF-8 encoding, but on Windows this is not correct
    return inputleap::fs::u8path(arg).filename().u8string();
}

} // namespace inputleap
