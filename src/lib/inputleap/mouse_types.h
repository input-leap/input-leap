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

#include "base/EventTypes.h"

//! Mouse button ID
/*!
Type to hold a mouse button identifier.
*/
typedef std::uint8_t ButtonID;

//! @name Mouse button identifiers
//@{
static const ButtonID    kButtonNone   = 0;
static const ButtonID    kButtonLeft   = 1;
static const ButtonID    kButtonMiddle = 2;
static const ButtonID    kButtonRight  = 3;
// mouse button 4
static const ButtonID    kButtonExtra0 = 4;
// mouse button 5
static const ButtonID    kButtonExtra1 = 5;

static const ButtonID   kMacButtonRight = 2;
static const ButtonID   kMacButtonMiddle = 3;
//@}

static const std::uint8_t NumButtonIDs  = 6;
