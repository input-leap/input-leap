/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "base/BitUtilities.h"
#include "platform/XWindowsClipboardBMPConverter.h"

namespace inputleap {

// BMP file header structure
struct CBMPHeader {
public:
    std::uint16_t type;
    std::uint32_t size;
    std::uint16_t reserved1;
    std::uint16_t reserved2;
    std::uint32_t offset;
};

XWindowsClipboardBMPConverter::XWindowsClipboardBMPConverter(
                Display* display) :
    m_atom(XInternAtom(display, "image/bmp", False))
{
    // do nothing
}

XWindowsClipboardBMPConverter::~XWindowsClipboardBMPConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardBMPConverter::getFormat() const
{
    return IClipboard::kBitmap;
}

Atom
XWindowsClipboardBMPConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardBMPConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardBMPConverter::fromIClipboard(const std::string& bmp) const
{
    if (bmp.empty()) {
        return {};
    }

    // create BMP image
    std::uint8_t header[14];
    std::uint8_t* dst = header;
    store_little_endian_u8(dst, 'B');
    store_little_endian_u8(dst, 'M');
    store_little_endian_u32(dst, 14 + bmp.size());
    store_little_endian_u16(dst, 0);
    store_little_endian_u16(dst, 0);
    store_little_endian_u32(dst, 14 + 40);
    return std::string(reinterpret_cast<const char*>(header), 14) + bmp;
}

std::string XWindowsClipboardBMPConverter::toIClipboard(const std::string& bmp) const
{
    if (bmp.empty()) {
        return {};
    }

    // make sure data is big enough for a BMP file
    if (bmp.size() <= 14 + 40) {
        return {};
    }

    // check BMP file header
    const std::uint8_t* rawBMPHeader = reinterpret_cast<const std::uint8_t*>(bmp.data());
    if (rawBMPHeader[0] != 'B' || rawBMPHeader[1] != 'M') {
        return {};
    }

    // get offset to image data
    std::uint32_t offset = load_little_endian_u32(rawBMPHeader + 10);

    // construct BMP
    if (offset == 14 + 40) {
        return bmp.substr(14);
    }
    else {
        return bmp.substr(14, 40) + bmp.substr(offset, bmp.size() - offset);
    }
}

} // namespace inputleap
