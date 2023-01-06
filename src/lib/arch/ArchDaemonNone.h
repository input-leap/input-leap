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

#pragma once

#include "arch/IArchDaemon.h"
#include <string>

#define ARCH_DAEMON ArchDaemonNone

//! Dummy implementation of IArchDaemon
/*!
This class implements IArchDaemon for a platform that does not have
daemons.  The install and uninstall functions do nothing, the query
functions return false, and \c daemonize() simply calls the passed
function and returns its result.
*/
class ArchDaemonNone : public IArchDaemon {
public:
    ArchDaemonNone();
    ~ArchDaemonNone() override;

    // IArchDaemon overrides
    void installDaemon(const char* name,
                       const char* description,
                       const char* pathname,
                       const char* commandLine,
                       const char* dependencies) override;
    void uninstallDaemon(const char* name) override;
    int daemonize(const char* name, DaemonFunc func) override;
    bool canInstallDaemon(const char* name) override;
    bool isDaemonInstalled(const char* name) override;
    void installDaemon() override;
    void uninstallDaemon() override;
    std::string commandLine() const override;
};
