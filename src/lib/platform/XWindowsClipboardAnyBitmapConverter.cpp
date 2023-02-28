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
#include "platform/XWindowsClipboardAnyBitmapConverter.h"

namespace inputleap {

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
    if (bmp.empty()) {
        return {};
    }

    // fill BMP info header with native-endian data
    CBMPInfoHeader infoHeader;
    const std::uint8_t* rawBMPInfoHeader = reinterpret_cast<const std::uint8_t*>(bmp.data());
    infoHeader.biSize             = load_little_endian_u32(rawBMPInfoHeader +  0);
    infoHeader.biWidth            = load_little_endian_s32(rawBMPInfoHeader +  4);
    infoHeader.biHeight           = load_little_endian_s32(rawBMPInfoHeader +  8);
    infoHeader.biPlanes           = load_little_endian_u16(rawBMPInfoHeader + 12);
    infoHeader.biBitCount         = load_little_endian_u16(rawBMPInfoHeader + 14);
    infoHeader.biCompression      = load_little_endian_u32(rawBMPInfoHeader + 16);
    infoHeader.biSizeImage        = load_little_endian_u32(rawBMPInfoHeader + 20);
    infoHeader.biXPelsPerMeter    = load_little_endian_s32(rawBMPInfoHeader + 24);
    infoHeader.biYPelsPerMeter    = load_little_endian_s32(rawBMPInfoHeader + 28);
    infoHeader.biClrUsed          = load_little_endian_u32(rawBMPInfoHeader + 32);
    infoHeader.biClrImportant     = load_little_endian_u32(rawBMPInfoHeader + 36);

    // check that format is acceptable
    if (infoHeader.biSize != 40 ||
        infoHeader.biWidth == 0 || infoHeader.biHeight == 0 ||
        infoHeader.biPlanes != 0 || infoHeader.biCompression != 0 ||
        (infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32)) {
        return {};
    }

    // convert to image format
    const std::uint8_t* rawBMPPixels = rawBMPInfoHeader + 40;
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
    if (image.empty()) {
        return {};
    }

    // convert to raw BMP data
    std::uint32_t w, h, depth;
    std::string rawBMP = doToIClipboard(image, w, h, depth);
    if (rawBMP.empty() || w == 0 || h == 0 || (depth != 24 && depth != 32)) {
        return {};
    }

    // fill BMP info header with little-endian data
    std::uint8_t infoHeader[40];
    std::uint8_t* dst = infoHeader;
    store_little_endian_u32(dst, 40);
    store_little_endian_s32(dst, w);
    store_little_endian_s32(dst, h);
    store_little_endian_u16(dst, 1);
    store_little_endian_u16(dst, depth);
    store_little_endian_u32(dst, 0);        // BI_RGB
    store_little_endian_u32(dst, image.size());
    store_little_endian_s32(dst, 2834);    // 72 dpi
    store_little_endian_s32(dst, 2834);    // 72 dpi
    store_little_endian_u32(dst, 0);
    store_little_endian_u32(dst, 0);

    // construct image
    return std::string(reinterpret_cast<const char*>(infoHeader),
                       sizeof(infoHeader)) + rawBMP;
}

} // namespace inputleap
