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

#include "inputleap/XBarrier.h"
#include "base/String.h"

//
// XBadClient
//

std::string XBadClient::getWhat() const noexcept
{
    return "XBadClient";
}


//
// XIncompatibleClient
//

XIncompatibleClient::XIncompatibleClient(int major, int minor) :
    m_major(major),
    m_minor(minor)
{
    // do nothing
}

int XIncompatibleClient::getMajor() const noexcept
{
    return m_major;
}

int XIncompatibleClient::getMinor() const noexcept
{
    return m_minor;
}

std::string XIncompatibleClient::getWhat() const noexcept
{
    return format("XIncompatibleClient", "incompatible client %{1}.%{2}",
                                inputleap::string::sprintf("%d", m_major).c_str(),
                                inputleap::string::sprintf("%d", m_minor).c_str());
}


//
// XDuplicateClient
//

XDuplicateClient::XDuplicateClient(const std::string& name) :
    m_name(name)
{
    // do nothing
}

const std::string& XDuplicateClient::getName() const noexcept
{
    return m_name;
}

std::string XDuplicateClient::getWhat() const noexcept
{
    return format("XDuplicateClient", "duplicate client %{1}", m_name.c_str());
}


//
// XUnknownClient
//

XUnknownClient::XUnknownClient(const std::string& name) :
    m_name(name)
{
    // do nothing
}

const std::string& XUnknownClient::getName() const noexcept
{
    return m_name;
}

std::string XUnknownClient::getWhat() const noexcept
{
    return format("XUnknownClient", "unknown client %{1}", m_name.c_str());
}


//
// XExitApp
//

XExitApp::XExitApp(int code) :
    m_code(code)
{
    // do nothing
}

int XExitApp::getCode() const noexcept
{
    return m_code;
}

std::string XExitApp::getWhat() const noexcept
{
    return format(
        "XExitApp", "exiting with code %{1}",
        inputleap::string::sprintf("%d", m_code).c_str());
}
