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

#pragma once

#include "inputleap/Chunk.h"

#include <cstdint>
#include <string>

#define FILE_CHUNK_META_SIZE 2

namespace inputleap {
class IStream;
}

class FileChunk : public Chunk {
public:
    FileChunk(size_t size);

    static FileChunk* start(const std::string& size);
    static FileChunk* data(std::uint8_t* data, size_t dataSize);
    static FileChunk* end();
    static int assemble(inputleap::IStream* stream, std::string& dataCached, size_t& expectedSize);
    static void send(inputleap::IStream* stream, std::uint8_t mark, char* data, size_t dataSize);
};
