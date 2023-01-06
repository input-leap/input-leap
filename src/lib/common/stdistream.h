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

#include "config.h"

#include "common/stdpre.h"
#include <istream>
#include "common/stdpost.h"

#if defined(_MSC_VER) && _MSC_VER <= 1200
// VC++6 istream has no overloads for __int* types, .NET does
inline
std::istream& operator>>(std::istream& s, std::int8_t& i)
{ return s >> (signed char&)i; }
inline
std::istream& operator>>(std::istream& s, std::int16_t& i)
{ return s >> (short&)i; }
inline
std::istream& operator>>(std::istream& s, std::int32_t& i)
{ return s >> (int&)i; }
inline
std::istream& operator>>(std::istream& s, std::uint8_t& i)
{ return s >> (unsigned char&)i; }
inline
std::istream& operator>>(std::istream& s, std::uint16_t& i)
{ return s >> (unsigned short&)i; }
inline
std::istream& operator>>(std::istream& s, std::uint32_t& i)
{ return s >> (unsigned int&)i; }
#endif
