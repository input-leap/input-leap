/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include <cstdint>

//
// make typedefs
//
// except for SInt8 and UInt8 these types are only guaranteed to be
// at least as big as indicated (in bits).  that is, they may be
// larger than indicated.
//

// Added this because it doesn't compile on OS X 10.6 because they are already defined in Carbon
#if !defined(__MACTYPES__)
#if defined(__APPLE__)
#include <CoreServices/CoreServices.h>
#else
using SInt8 = std::int8_t;
using SInt16 = std::int16_t;
using SInt32 = std::int32_t;
using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
#endif
#endif
