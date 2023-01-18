/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2015-2016 Symless Ltd.
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

#pragma once

#include "inputleap/Chunk.h"
#include "inputleap/clipboard_types.h"

#include <cstdint>
#include <string>

#define CLIPBOARD_CHUNK_META_SIZE 7

namespace inputleap {

class IStream;

class ClipboardChunk : public Chunk {
public:
    ClipboardChunk(size_t size);

    static ClipboardChunk start(ClipboardID id, std::uint32_t sequence, const std::size_t& size);
    static ClipboardChunk data(ClipboardID id, std::uint32_t sequence, const std::string& data);
    static ClipboardChunk end(ClipboardID id, std::uint32_t sequence);

    static int assemble(inputleap::IStream* stream, std::string& dataCached, ClipboardID& id,
                        std::uint32_t& sequence);

    static void send(inputleap::IStream* stream, const ClipboardChunk& clipboard_data);

    static size_t getExpectedSize() { return s_expectedSize; }

private:
    static size_t        s_expectedSize;
};

} // namespace inputleap
