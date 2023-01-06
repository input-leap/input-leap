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

#include "common/stdstring.h"

//! Interface for architecture dependent system queries
/*!
This interface defines operations for querying system info.
*/
class IArchSystem {
public:
    virtual ~IArchSystem() { }

    //! @name accessors
    //@{

    //! Identify the OS
    /*!
    Returns a string identifying the operating system.
    */
    virtual std::string getOSName() const = 0;

    //! Identify the platform
    /*!
    Returns a string identifying the platform this OS is running on.
    */
    virtual std::string getPlatformName() const = 0;
    //@}

    //! Get a Barrier setting
    /*!
    Reads a Barrier setting from the system.
    */
    virtual std::string setting(const std::string& valueName) const = 0;
    //@}

    //! Set a Barrier setting
    /*!
    Writes a Barrier setting from the system.
    */
    virtual void setting(const std::string& valueName, const std::string& valueString) const = 0;
    //@}

    //! Get the pathnames of the libraries used by Barrier
    /*
    Returns a string containing the full path names of all loaded libraries at the point it is called.
    */
    virtual std::string getLibsUsed(void) const = 0;
    //@}
};
