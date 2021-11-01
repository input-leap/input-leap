/*
    barrier -- mouse and keyboard sharing utility
    Copyright (C) Barrier contributors

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

#include "fstream.h"
#if SYSAPI_WIN32
#include "common/win32/encoding_utilities.h"
#endif
#include <fstream>

namespace barrier {

namespace {

template<class Stream>
void open_utf8_path_impl(Stream& stream, const std::string& path, std::ios_base::openmode mode)
{
#if SYSAPI_WIN32
    // on Windows we need to use a private constructor from wchar_t* string.
    auto wchar_path = utf8_to_win_char(path);
    stream.open(wchar_path.data(), mode);
#else
    stream.open(path.c_str(), mode);
#endif
}

} // namespace

void open_utf8_path(std::ifstream& stream, const std::string& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

void open_utf8_path(std::ofstream& stream, const std::string& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

void open_utf8_path(std::fstream& stream, const std::string& path, std::ios_base::openmode mode)
{
    open_utf8_path_impl(stream, path, mode);
}

std::FILE* fopen_utf8_path(const std::string& path, const std::string& mode)
{
#if SYSAPI_WIN32
    auto wchar_path = utf8_to_win_char(path);
    auto wchar_mode = utf8_to_win_char(mode);
    return _wfopen(reinterpret_cast<wchar_t*>(wchar_path.data()),
                   reinterpret_cast<wchar_t*>(wchar_mode.data()));
#else
    return std::fopen(path.c_str(), mode.c_str());
#endif
}

} // namespace barrier
