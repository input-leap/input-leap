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

#include "platform/MSWindowsClipboardUTF16Converter.h"

#include "base/Unicode.h"

//
// MSWindowsClipboardUTF16Converter
//

MSWindowsClipboardUTF16Converter::MSWindowsClipboardUTF16Converter()
{
    // do nothing
}

MSWindowsClipboardUTF16Converter::~MSWindowsClipboardUTF16Converter()
{
    // do nothing
}

UINT
MSWindowsClipboardUTF16Converter::getWin32Format() const
{
    return CF_UNICODETEXT;
}

std::string MSWindowsClipboardUTF16Converter::doFromIClipboard(const std::string& data) const
{
    // convert and add nul terminator
    return Unicode::UTF8ToUTF16(data).append(sizeof(wchar_t), 0);
}

std::string MSWindowsClipboardUTF16Converter::doToIClipboard(const std::string& data) const
{
    // convert and strip nul terminator
    std::string dst = Unicode::UTF16ToUTF8(data);
    std::string::size_type n = dst.find('\0');
    if (n != std::string::npos) {
        dst.erase(n);
    }
    return dst;
}
