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

ArgsBase* ArgParser::m_argsBase = nullptr;

ArgParser::ArgParser(App* app) :
    m_app(app)
{
}

bool
ArgParser::parseServerArgs(ServerArgs& args, int argc, const char* const* argv)
{
    setArgsBase(args);
    updateCommonArgs(argv);

    for (int i = 1; i < argc; ++i) {
        if (parsePlatformArg(args, argc, argv, i)) {
            continue;
        }
        else if (parseGenericArgs(argc, argv, i)) {
            continue;
        }
        else if (parseDeprecatedArgs(argc, argv, i)) {
            continue;
        }
        else if (isArg(i, argc, argv, "-a", "--address", 1)) {
            // save listen address
            args.m_barrierAddress = argv[++i];
        }
        else if (isArg(i, argc, argv, "-c", "--config", 1)) {
            // save configuration file path
            args.m_configFile = argv[++i];
        }
        else if (isArg(i, argc, argv, nullptr, "--screen-change-script", 1)) {
            // save screen change script path
            args.m_screenChangeScript = argv[++i];
        }
        else if (isArg(i, argc, argv, nullptr, "--disable-client-cert-checking")) {
            args.check_client_certificates = false;
        } else {
            LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE, args.m_exename.c_str(), argv[i], args.m_exename.c_str()));
            return false;
        }
    }

    if (checkUnexpectedArgs()) {
        return false;
    }

    return true;
}

bool
ArgParser::parseClientArgs(ClientArgs& args, int argc, const char* const* argv)
{
    setArgsBase(args);
    updateCommonArgs(argv);

    int i;
    for (i = 1; i < argc; ++i) {
        if (parsePlatformArg(args, argc, argv, i)) {
            continue;
        }
        else if (parseGenericArgs(argc, argv, i)) {
            continue;
        }
        else if (parseDeprecatedArgs(argc, argv, i)) {
            continue;
        }
        else if (isArg(i, argc, argv, nullptr, "--camp")) {
            // ignore -- included for backwards compatibility
        }
        else if (isArg(i, argc, argv, nullptr, "--no-camp")) {
            // ignore -- included for backwards compatibility
        }
        else if (isArg(i, argc, argv, nullptr, "--yscroll", 1)) {
            // define scroll
            args.m_yscroll = atoi(argv[++i]);
        }
        else {
            if (i + 1 == argc) {
                args.m_barrierAddress = argv[i];
                return true;
            }

            LOG((CLOG_PRINT "%s: unrecognized option `%s'" BYE, args.m_exename.c_str(), argv[i], args.m_exename.c_str()));
            return false;
        }
    }

    if (args.m_shouldExit)
        return true;

    // exactly one non-option argument (server-address)
    if (i == argc) {
        LOG((CLOG_PRINT "%s: a server address or name is required" BYE,
            args.m_exename.c_str(), args.m_exename.c_str()));
        return false;
    }

    if (checkUnexpectedArgs()) {
        return false;
    }

    return true;
}

#if WINAPI_MSWINDOWS
bool
ArgParser::parseMSWindowsArg(ArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
    if (isArg(i, argc, argv, nullptr, "--service")) {
        LOG((CLOG_WARN "obsolete argument --service, use barrierd instead."));
        argsBase.m_shouldExit = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--exit-pause")) {
        argsBase.m_pauseOnExit = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--stop-on-desk-switch")) {
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
ArgParser::parseCarbonArg(ArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
    // no options for carbon
    return false;
}
#endif

#if WINAPI_XWINDOWS
bool
ArgParser::parseXWindowsArg(ArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
    if (isArg(i, argc, argv, "-display", "--display", 1)) {
        // use alternative display
        argsBase.m_display = argv[++i];
    }
    else if (isArg(i, argc, argv, nullptr, "--no-xinitthreads")) {
        LOG((CLOG_NOTE "--no-xinitthreads is deprecated"));
    } else {
        // option not supported here
        return false;
    }

    return true;
}
#endif

bool
ArgParser::parsePlatformArg(ArgsBase& argsBase, const int& argc, const char* const* argv, int& i)
{
#if WINAPI_MSWINDOWS
    return parseMSWindowsArg(argsBase, argc, argv, i);
#endif
#if WINAPI_CARBON
    return parseCarbonArg(argsBase, argc, argv, i);
#endif
#if WINAPI_XWINDOWS
    return parseXWindowsArg(argsBase, argc, argv, i);
#endif
    return false;
}

bool
ArgParser::parseGenericArgs(int argc, const char* const* argv, int& i)
{
    if (isArg(i, argc, argv, "-d", "--debug", 1)) {
        // change logging level
        argsBase().m_logFilter = argv[++i];
    }
    else if (isArg(i, argc, argv, "-l", "--log", 1)) {
        argsBase().m_logFile = argv[++i];
    }
    else if (isArg(i, argc, argv, "-f", "--no-daemon")) {
        // not a daemon
        argsBase().m_daemon = false;
    }
    else if (isArg(i, argc, argv, nullptr, "--daemon")) {
        // daemonize
        argsBase().m_daemon = true;
    }
    else if (isArg(i, argc, argv, "-n", "--name", 1)) {
        // save screen name
        argsBase().m_name = argv[++i];
    }
    else if (isArg(i, argc, argv, "-1", "--no-restart")) {
        // don't try to restart
        argsBase().m_restartable = false;
    }
    else if (isArg(i, argc, argv, nullptr, "--restart")) {
        // try to restart
        argsBase().m_restartable = true;
    }
    else if (isArg(i, argc, argv, "-z", nullptr)) {
        argsBase().m_backend = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--no-hooks")) {
        argsBase().m_noHooks = true;
    }
    else if (isArg(i, argc, argv, "-h", "--help")) {
        if (m_app) {
            m_app->help();
        }
        argsBase().m_shouldExit = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--version")) {
        if (m_app) {
            m_app->version();
        }
        argsBase().m_shouldExit = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--no-tray")) {
        argsBase().m_disableTray = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--ipc")) {
        argsBase().m_enableIpc = true;
    }
    else if (isArg(i, argc, argv, nullptr, "--server")) {
        // HACK: stop error happening when using portable (barrierp)
    }
    else if (isArg(i, argc, argv, nullptr, "--client")) {
        // HACK: stop error happening when using portable (barrierp)
    }
    else if (isArg(i, argc, argv, nullptr, "--enable-drag-drop")) {
        bool useDragDrop = true;

#ifdef WINAPI_XWINDOWS

        useDragDrop = false;
        LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported on linux."));

#endif

#ifdef WINAPI_MSWINDOWS

        if (!IsWindowsVistaOrGreater()) {
            useDragDrop = false;
            LOG((CLOG_INFO "ignoring --enable-drag-drop, not supported below vista."));
        }
#endif

        if (useDragDrop) {
            argsBase().m_enableDragDrop = true;
        }
    }
    else if (isArg(i, argc, argv, nullptr, "--drop-dir")) {
        argsBase().m_dropTarget = argv[++i];
    }
    else if (isArg(i, argc, argv, nullptr, "--enable-crypto")) {
        LOG((CLOG_INFO "--enable-crypto is used by default. The option is deprecated."));
    }
    else if (isArg(i, argc, argv, nullptr, "--disable-crypto")) {
        argsBase().m_enableCrypto = false;
    }
    else if (isArg(i, argc, argv, nullptr, "--profile-dir", 1)) {
        argsBase().m_profileDirectory = inputleap::fs::u8path(argv[++i]);
    }
    else if (isArg(i, argc, argv, nullptr, "--plugin-dir", 1)) {
        argsBase().m_pluginDirectory = inputleap::fs::u8path(argv[++i]);
    }
    else {
        // option not supported here
        return false;
    }

    return true;
}

bool
ArgParser::parseDeprecatedArgs(int argc, const char* const* argv, int& i)
{
    if (isArg(i, argc, argv, nullptr, "--crypto-pass")) {
        LOG((CLOG_NOTE "--crypto-pass is deprecated"));
        i++;
        return true;
    }
    else if (isArg(i, argc, argv, nullptr, "--res-w")) {
        LOG((CLOG_NOTE "--res-w is deprecated"));
        i++;
        return true;
    }
    else if (isArg(i, argc, argv, nullptr, "--res-h")) {
        LOG((CLOG_NOTE "--res-h is deprecated"));
        i++;
        return true;
    }
    else if (isArg(i, argc, argv, nullptr, "--prm-wc")) {
        LOG((CLOG_NOTE "--prm-wc is deprecated"));
        i++;
        return true;
    }
    else if (isArg(i, argc, argv, nullptr, "--prm-hc")) {
        LOG((CLOG_NOTE "--prm-hc is deprecated"));
        i++;
        return true;
    }

    return false;
}

bool
ArgParser::isArg(
    int argi, int argc, const char* const* argv,
    const char* name1, const char* name2,
    int minRequiredParameters)
{
    if ((name1 != nullptr && strcmp(argv[argi], name1) == 0) ||
        (name2 != nullptr && strcmp(argv[argi], name2) == 0)) {
            // match.  check args left.
            if (argi + minRequiredParameters >= argc) {
                LOG((CLOG_PRINT "%s: missing arguments for `%s'" BYE,
                    argsBase().m_exename.c_str(), argv[argi], argsBase().m_exename.c_str()));
                argsBase().m_shouldExit = true;
                return false;
            }
            return true;
    }

    // no match
    return false;
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
ArgParser::updateCommonArgs(const char* const* argv)
{
    argsBase().m_name = ARCH->getHostName();
    argsBase().m_exename = parse_exename(argv[0]);
}

std::string ArgParser::parse_exename(const char* arg)
{
    // FIXME: we assume UTF-8 encoding, but on Windows this is not correct
    return inputleap::fs::u8path(arg).filename().u8string();
}

bool
ArgParser::checkUnexpectedArgs()
{
#if SYSAPI_WIN32
    // suggest that user installs as a windows service. when launched as
    // service, process should automatically detect that it should run in
    // daemon mode.
    if (argsBase().m_daemon) {
        LOG((CLOG_ERR
            "the --daemon argument is not supported on windows. "
            "instead, install %s as a service (--service install)",
            argsBase().m_exename.c_str()));
        return true;
    }
#endif

    return false;
}
