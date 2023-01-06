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

#ifndef INPUTLEAP_LIB_IO_FILESYSTEM_H
#define INPUTLEAP_LIB_IO_FILESYSTEM_H

#include <cstdio>
#include <iosfwd>
#include <ios>
#include <ghc/fs_fwd.hpp>

namespace inputleap {

namespace fs = ghc::filesystem;

void open_utf8_path(std::ifstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::in);
void open_utf8_path(std::ofstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::out);
void open_utf8_path(std::fstream& stream, const fs::path& path,
                    std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

std::FILE* fopen_utf8_path(const fs::path& path, const std::string& mode);

} // namespace inputleap

#endif // INPUTLEAP_LIB_IO_FILESYSTEM_H
