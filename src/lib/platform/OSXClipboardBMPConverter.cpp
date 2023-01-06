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

#include "platform/OSXClipboardBMPConverter.h"
#include "base/Log.h"

// BMP file header structure
struct CBMPHeader {
public:
    std::uint16_t type;
    std::uint32_t size;
    std::uint16_t reserved1;
    std::uint16_t reserved2;
    std::uint32_t offset;
};

// BMP is little-endian
static inline std::uint32_t fromLEU32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) << 8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24);
}

static void toLE(std::uint8_t*& dst, char src)
{
    dst[0] = static_cast<std::uint8_t>(src);
    dst += 1;
}

static void toLE(std::uint8_t*& dst, std::uint16_t src)
{
    dst[0] = static_cast<std::uint8_t>(src & 0xffu);
    dst[1] = static_cast<std::uint8_t>((src >> 8) & 0xffu);
    dst += 2;
}

static void toLE(std::uint8_t*& dst, std::uint32_t src)
{
    dst[0] = static_cast<std::uint8_t>(src & 0xffu);
    dst[1] = static_cast<std::uint8_t>((src >> 8) & 0xffu);
    dst[2] = static_cast<std::uint8_t>((src >> 16) & 0xffu);
    dst[3] = static_cast<std::uint8_t>((src >> 24) & 0xffu);
    dst += 4;
}

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
    toLE(dst, 'B');
    toLE(dst, 'M');
    toLE(dst, static_cast<std::uint32_t>(14 + bmp.size()));
    toLE(dst, static_cast<std::uint16_t>(0));
    toLE(dst, static_cast<std::uint16_t>(0));
    toLE(dst, static_cast<std::uint32_t>(14 + 40));
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
    std::uint32_t offset = fromLEU32(rawBMPHeader + 10);

    // construct BMP
    if (offset == 14 + 40) {
        return bmp.substr(14);
    }
    else {
        return bmp.substr(14, 40) + bmp.substr(offset, bmp.size() - offset);
    }
}
