/*
 * InputLeap -- mouse and keyboard sharing utility
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

#include "io/XIO.h"
#include "base/EventTypes.h"

#include <stdarg.h>

namespace inputleap { class IStream; }

//! Barrier protocol utilities
/*!
This class provides various functions for implementing the barrier
protocol.
*/
class ProtocolUtil {
public:
    //! Write formatted data
    /*!
    Write formatted binary data to a stream.  \c fmt consists of
    regular characters and format specifiers.  Format specifiers
    begin with \%.  All characters not part of a format specifier
    are regular and are transmitted unchanged.

    Format specifiers are:
    - \%\%   -- literal `\%'
    - \%1i  -- converts integer argument to 1 byte integer
    - \%2i  -- converts integer argument to 2 byte integer in NBO
    - \%4i  -- converts integer argument to 4 byte integer in NBO
    - \%1I  -- converts std::vector<UInt8>* to 1 byte integers
    - \%2I  -- converts std::vector<std::uint16_t>* to 2 byte integers in NBO
    - \%4I  -- converts std::vector<std::uint32_t>* to 4 byte integers in NBO
    - \%s   -- converts std::string* to stream of bytes
    - \%S   -- converts integer N and const UInt8* to stream of N bytes
    */
    static void writef(inputleap::IStream*, const char* fmt, ...);

    //! Read formatted data
    /*!
    Read formatted binary data from a buffer.  This performs the
    reverse operation of writef().  Returns true if the entire
    format was successfully parsed, false otherwise.

    Format specifiers are:
    - \%\%   -- read (and discard) a literal `\%'
    - \%1i  -- reads a 1 byte integer; argument is a std::int32_t* or std::uint32_t*
    - \%2i  -- reads an NBO 2 byte integer;  arg is std::int32_t* or std::uint32_t*
    - \%4i  -- reads an NBO 4 byte integer;  arg is std::int32_t* or std::uint32_t*
    - \%1I  -- reads 1 byte integers;  arg is std::vector<UInt8>*
    - \%2I  -- reads NBO 2 byte integers;  arg is std::vector<std::uint16_t>*
    - \%4I  -- reads NBO 4 byte integers;  arg is std::vector<std::uint32_t>*
    - \%s   -- reads bytes;  argument must be a std::string*, \b not a char*
    */
    static bool readf(inputleap::IStream*, const char* fmt, ...);

private:
    static void vwritef(inputleap::IStream*, const char* fmt, std::uint32_t size, va_list);
    static void vreadf(inputleap::IStream*, const char* fmt, va_list);

    static std::uint32_t getLength(const char* fmt, va_list);
    static void            writef_void(void*, const char* fmt, va_list);
    static std::uint32_t eatLength(const char** fmt);
    static void read(inputleap::IStream*, void*, std::uint32_t);
};

//! Mismatched read exception
/*!
Thrown by ProtocolUtil::readf() when the data being read does not
match the format.
*/
class XIOReadMismatch : public XIO {
public:
    // XBase overrides
    std::string getWhat() const noexcept override;
};
