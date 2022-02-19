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

#include "platform/XWindowsClipboardAnyBitmapConverter.h"

// BMP info header structure
struct CBMPInfoHeader {
public:
    std::uint32_t biSize;
    std::int32_t biWidth;
    std::int32_t biHeight;
    std::uint16_t biPlanes;
    std::uint16_t biBitCount;
    std::uint32_t biCompression;
    std::uint32_t biSizeImage;
    std::int32_t biXPelsPerMeter;
    std::int32_t biYPelsPerMeter;
    std::uint32_t biClrUsed;
    std::uint32_t biClrImportant;
};

// BMP is little-endian

static void toLE(UInt8*& dst, std::uint16_t src)
{
    dst[0] = static_cast<UInt8>(src & 0xffu);
    dst[1] = static_cast<UInt8>((src >> 8) & 0xffu);
    dst += 2;
}

static void toLE(UInt8*& dst, std::int32_t src)
{
    dst[0] = static_cast<UInt8>(src & 0xffu);
    dst[1] = static_cast<UInt8>((src >>  8) & 0xffu);
    dst[2] = static_cast<UInt8>((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8>((src >> 24) & 0xffu);
    dst += 4;
}

static void toLE(UInt8*& dst, std::uint32_t src)
{
    dst[0] = static_cast<UInt8>(src & 0xffu);
    dst[1] = static_cast<UInt8>((src >>  8) & 0xffu);
    dst[2] = static_cast<UInt8>((src >> 16) & 0xffu);
    dst[3] = static_cast<UInt8>((src >> 24) & 0xffu);
    dst += 4;
}

static inline std::uint16_t fromLEU16(const UInt8* data)
{
    return static_cast<std::uint16_t>(data[0]) |
            (static_cast<std::uint16_t>(data[1]) << 8);
}

static inline std::int32_t fromLES32(const UInt8* data)
{
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) <<  8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24));
}

static inline std::uint32_t fromLEU32(const UInt8* data)
{
    return static_cast<std::uint32_t>(data[0]) |
            (static_cast<std::uint32_t>(data[1]) <<  8) |
            (static_cast<std::uint32_t>(data[2]) << 16) |
            (static_cast<std::uint32_t>(data[3]) << 24);
}


//
// XWindowsClipboardAnyBitmapConverter
//

XWindowsClipboardAnyBitmapConverter::XWindowsClipboardAnyBitmapConverter()
{
    // do nothing
}

XWindowsClipboardAnyBitmapConverter::~XWindowsClipboardAnyBitmapConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardAnyBitmapConverter::getFormat() const
{
    return IClipboard::kBitmap;
}

int
XWindowsClipboardAnyBitmapConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardAnyBitmapConverter::fromIClipboard(const std::string& bmp) const
{
    // fill BMP info header with native-endian data
    CBMPInfoHeader infoHeader;
    const UInt8* rawBMPInfoHeader = reinterpret_cast<const UInt8*>(bmp.data());
    infoHeader.biSize             = fromLEU32(rawBMPInfoHeader +  0);
    infoHeader.biWidth            = fromLES32(rawBMPInfoHeader +  4);
    infoHeader.biHeight           = fromLES32(rawBMPInfoHeader +  8);
    infoHeader.biPlanes           = fromLEU16(rawBMPInfoHeader + 12);
    infoHeader.biBitCount         = fromLEU16(rawBMPInfoHeader + 14);
    infoHeader.biCompression      = fromLEU32(rawBMPInfoHeader + 16);
    infoHeader.biSizeImage        = fromLEU32(rawBMPInfoHeader + 20);
    infoHeader.biXPelsPerMeter    = fromLES32(rawBMPInfoHeader + 24);
    infoHeader.biYPelsPerMeter    = fromLES32(rawBMPInfoHeader + 28);
    infoHeader.biClrUsed          = fromLEU32(rawBMPInfoHeader + 32);
    infoHeader.biClrImportant     = fromLEU32(rawBMPInfoHeader + 36);

    // check that format is acceptable
    if (infoHeader.biSize != 40 ||
        infoHeader.biWidth == 0 || infoHeader.biHeight == 0 ||
        infoHeader.biPlanes != 0 || infoHeader.biCompression != 0 ||
        (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32)) {
        return {};
    }

    // convert to image format
    const UInt8* rawBMPPixels = rawBMPInfoHeader + 40;
    if (infoHeader.biBitCount == 24) {
        return doBGRFromIClipboard(rawBMPPixels,
                            infoHeader.biWidth, infoHeader.biHeight);
    }
    else {
        return doBGRAFromIClipboard(rawBMPPixels,
                            infoHeader.biWidth, infoHeader.biHeight);
    }
}

std::string XWindowsClipboardAnyBitmapConverter::toIClipboard(const std::string& image) const
{
    // convert to raw BMP data
    std::uint32_t w, h, depth;
    std::string rawBMP = doToIClipboard(image, w, h, depth);
    if (rawBMP.empty() || w == 0 || h == 0 || (depth != 24 && depth != 32)) {
        return {};
    }

    // fill BMP info header with little-endian data
    UInt8 infoHeader[40];
    UInt8* dst = infoHeader;
    toLE(dst, static_cast<std::uint32_t>(40));
    toLE(dst, static_cast<std::int32_t>(w));
    toLE(dst, static_cast<std::int32_t>(h));
    toLE(dst, static_cast<std::uint16_t>(1));
    toLE(dst, static_cast<std::uint16_t>(depth));
    toLE(dst, static_cast<std::uint32_t>(0));        // BI_RGB
    toLE(dst, static_cast<std::uint32_t>(image.size()));
    toLE(dst, static_cast<std::int32_t>(2834));    // 72 dpi
    toLE(dst, static_cast<std::int32_t>(2834));    // 72 dpi
    toLE(dst, static_cast<std::uint32_t>(0));
    toLE(dst, static_cast<std::uint32_t>(0));

    // construct image
    return std::string(reinterpret_cast<const char*>(infoHeader),
                       sizeof(infoHeader)) + rawBMP;
}
