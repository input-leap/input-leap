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

#ifndef BARRIER_LIB_IO_FSTREAM_H
#define BARRIER_LIB_IO_FSTREAM_H

#include <iosfwd>
#include <ios>

namespace barrier {

void open_utf8_path(std::ifstream& stream, const std::string& path,
                    std::ios_base::openmode mode = std::ios_base::in);
void open_utf8_path(std::ofstream& stream, const std::string& path,
                    std::ios_base::openmode mode = std::ios_base::out);
void open_utf8_path(std::fstream& stream, const std::string& path,
                    std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);

} // namespace barrier

#endif
