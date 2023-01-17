/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
 * Patch by Ryan Chapman
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
#include "platform/OSXClipboardBMPConverter.h"
#include "base/Log.h"

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

OSXClipboardBMPConverter::OSXClipboardBMPConverter()
{
    // do nothing
}

OSXClipboardBMPConverter::~OSXClipboardBMPConverter()
{
    // do nothing
}

IClipboard::EFormat
OSXClipboardBMPConverter::getFormat() const
{
    return IClipboard::kBitmap;
}

CFStringRef
OSXClipboardBMPConverter::getOSXFormat() const
{
    // TODO: does this only work with Windows?
    return CFSTR("com.microsoft.bmp");
}

std::string OSXClipboardBMPConverter::fromIClipboard(const std::string& bmp) const
{
    LOG((CLOG_DEBUG1 "ENTER OSXClipboardBMPConverter::doFromIClipboard()"));
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

std::string OSXClipboardBMPConverter::toIClipboard(const std::string& bmp) const
{
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
