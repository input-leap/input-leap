/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) 2012-2016 Symless Ltd.
    Copyright (C) 2004 Chris Schoeneman

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>

namespace inputleap {

inline void store_little_endian_u8(std::uint8_t*& dst, std::uint8_t src)
{
    dst[0] = src;
    dst += 1;
}

inline void store_little_endian_s8(std::uint8_t*& dst, std::int8_t src)
{
    dst[0] = static_cast<std::uint8_t>(src);
    dst += 1;
}

inline void store_little_endian_u16(std::uint8_t*& dst, std::uint16_t src)
{
    dst[0] = static_cast<std::uint8_t>(src & 0xffu);
    dst[1] = static_cast<std::uint8_t>((src >> 8) & 0xffu);
    dst += 2;
}

inline void store_little_endian_s32(std::uint8_t*& dst, std::int32_t src)
{
    dst[0] = static_cast<std::uint8_t>(src & 0xffu);
    dst[1] = static_cast<std::uint8_t>((src >>  8) & 0xffu);
    dst[2] = static_cast<std::uint8_t>((src >> 16) & 0xffu);
    dst[3] = static_cast<std::uint8_t>((src >> 24) & 0xffu);
    dst += 4;
}

inline void store_little_endian_u32(std::uint8_t*& dst, std::uint32_t src)
{
    dst[0] = static_cast<std::uint8_t>(src & 0xffu);
    dst[1] = static_cast<std::uint8_t>((src >>  8) & 0xffu);
    dst[2] = static_cast<std::uint8_t>((src >> 16) & 0xffu);
    dst[3] = static_cast<std::uint8_t>((src >> 24) & 0xffu);
    dst += 4;
}

inline std::uint16_t load_little_endian_u16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0]) |
            (static_cast<std::uint16_t>(data[1]) << 8);
}

inline std::int32_t load_little_endian_s32(const std::uint8_t* data)
{
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) <<  8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24));
}

inline std::uint32_t load_little_endian_u32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) <<  8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24);
}

} // namespace inputleap
