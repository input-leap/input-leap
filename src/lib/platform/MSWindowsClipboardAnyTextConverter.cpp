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

#include "platform/MSWindowsClipboardAnyTextConverter.h"

//
// MSWindowsClipboardAnyTextConverter
//

MSWindowsClipboardAnyTextConverter::MSWindowsClipboardAnyTextConverter()
{
    // do nothing
}

MSWindowsClipboardAnyTextConverter::~MSWindowsClipboardAnyTextConverter()
{
    // do nothing
}

IClipboard::EFormat
MSWindowsClipboardAnyTextConverter::getFormat() const
{
    return IClipboard::kText;
}

HANDLE MSWindowsClipboardAnyTextConverter::fromIClipboard(const std::string& data) const
{
    // convert linefeeds and then convert to desired encoding
    std::string text = doFromIClipboard(convertLinefeedToWin32(data));
    std::uint32_t size = (std::uint32_t)text.size();

    // copy to memory handle
    HGLOBAL gData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, size);
    if (gData != nullptr) {
        // get a pointer to the allocated memory
        char* dst = (char*)GlobalLock(gData);
        if (dst != nullptr) {
            memcpy(dst, text.data(), size);
            GlobalUnlock(gData);
        }
        else {
            GlobalFree(gData);
            gData = nullptr;
        }
    }

    return gData;
}

std::string MSWindowsClipboardAnyTextConverter::toIClipboard(HANDLE data) const
{
    // get datator
    const char* src = (const char*)GlobalLock(data);
    std::uint32_t srcSize = (std::uint32_t)GlobalSize(data);
    if (src == nullptr || srcSize <= 1) {
        return {};
    }

    // convert text
    std::string text = doToIClipboard(std::string(src, srcSize));

    // release handle
    GlobalUnlock(data);

    // convert newlines
    return convertLinefeedToUnix(text);
}

std::string MSWindowsClipboardAnyTextConverter::convertLinefeedToWin32(const std::string& src) const
{
    // note -- we assume src is a valid UTF-8 string

    // count newlines in string
    std::uint32_t numNewlines = 0;
    std::uint32_t n = (std::uint32_t)src.size();
    for (const char* scan = src.c_str(); n > 0; ++scan, --n) {
        if (*scan == '\n') {
            ++numNewlines;
        }
    }
    if (numNewlines == 0) {
        return src;
    }

    // allocate new string
    std::string dst;
    dst.reserve(src.size() + numNewlines);

    // copy string, converting newlines
    n = (std::uint32_t)src.size();
    for (const char* scan = src.c_str(); n > 0; ++scan, --n) {
        if (scan[0] == '\n') {
            dst += '\r';
        }
        dst += scan[0];
    }

    return dst;
}

std::string MSWindowsClipboardAnyTextConverter::convertLinefeedToUnix(const std::string& src) const
{
    // count newlines in string
    std::uint32_t numNewlines = 0;
    std::uint32_t n = (std::uint32_t)src.size();
    for (const char* scan = src.c_str(); n > 0; ++scan, --n) {
        if (scan[0] == '\r' && scan[1] == '\n') {
            ++numNewlines;
        }
    }
    if (numNewlines == 0) {
        return src;
    }

    // allocate new string
    std::string dst;
    dst.reserve(src.size());

    // copy string, converting newlines
    n = (std::uint32_t)src.size();
    for (const char* scan = src.c_str(); n > 0; ++scan, --n) {
        if (scan[0] != '\r' || scan[1] != '\n') {
            dst += scan[0];
        }
    }

    return dst;
}
