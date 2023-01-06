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

#include <string>

#define WINDOWS_LEAN_AND_MEAN
#include <Windows.h>

class MSWindowsUtil {
public:
    //! Get message string
    /*!
    Gets a string for \p id from the string table of \p instance.
    */
    static std::string getString(HINSTANCE instance, DWORD id);

    //! Get error string
    /*!
    Gets a system error message for \p error.  If the error cannot be
    found return the string for \p id, replacing ${1} with \p error.
    */
    static std::string getErrorString(HINSTANCE, DWORD error, DWORD id);

    //! Create directory
    /*!
    Create directory \p recursively optionally stripping the last component
    (presumably the filename) if \p is set.
    */
    static void createDirectory(const std::string& path, bool stripLast = false);
};
