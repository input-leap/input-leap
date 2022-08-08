/*
    InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

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

#include "encoding_utilities.h"
#include <stringapiset.h>
#include <limits>

std::string win_wchar_to_utf8(const WCHAR* utfStr)
{
    int utfLength = lstrlenW(utfStr);
    int mbLength = WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, nullptr, 0, nullptr, nullptr);
    std::string mbStr(mbLength, 0);
    WideCharToMultiByte(CP_UTF8, 0, utfStr, utfLength, &mbStr[0], mbLength, nullptr, nullptr);
    return mbStr;
}

std::vector<WCHAR> utf8_to_win_char(const std::string& str)
{
    if (str.size() > std::numeric_limits<int>::max())
        return {};
    int input_len = static_cast<int>(str.size());
    int result_len = MultiByteToWideChar(CP_UTF8, 0, str.data(), input_len, nullptr, 0);
    std::vector<WCHAR> result;
    result.resize(result_len + 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.data(), input_len, result.data(), result_len);
    return result;
}
