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

#include "base/XBase.h"

//! Generic screen exception
XBASE_SUBCLASS(XScreen, XBase);

//! Cannot open screen exception
/*!
Thrown when a screen cannot be opened or initialized.
*/
XBASE_SUBCLASS_WHAT(XScreenOpenFailure, XScreen);

//! XInput exception
/*!
Thrown when an XInput error occurs
*/
XBASE_SUBCLASS_WHAT(XScreenXInputFailure, XScreen);

//! Screen unavailable exception
/*!
Thrown when a screen cannot be opened or initialized but retrying later
may be successful.
*/
class XScreenUnavailable : public XScreenOpenFailure {
public:
    /*!
    \c timeUntilRetry is the suggested time the caller should wait until
    trying to open the screen again.
    */
    XScreenUnavailable(double timeUntilRetry);
    ~XScreenUnavailable() noexcept override;

    //! @name manipulators
    //@{

    //! Get retry time
    /*!
    Returns the suggested time to wait until retrying to open the screen.
    */
    double getRetryTime() const;

    //@}

protected:
    std::string getWhat() const noexcept override;

private:
    double m_timeUntilRetry;
};
