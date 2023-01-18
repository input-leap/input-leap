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

#include "inputleap/ClipboardChunk.h"

#include "inputleap/ProtocolUtil.h"
#include "inputleap/protocol_types.h"
#include "io/IStream.h"
#include "base/Log.h"
#include "base/String.h"
#include <cstring>

namespace inputleap {

size_t ClipboardChunk::s_expectedSize = 0;

ClipboardChunk ClipboardChunk::start(ClipboardID id, std::uint32_t sequence,
                                     const std::size_t& size)
{
    ClipboardChunk chunk;
    chunk.id_ = id;
    chunk.sequence_ = sequence;
    chunk.mark_ = kDataStart;
    chunk.data_ = std::to_string(size);
    return chunk;
}

ClipboardChunk ClipboardChunk::data(ClipboardID id, std::uint32_t sequence,
                                    const std::string& data)
{
    ClipboardChunk chunk;
    chunk.id_ = id;
    chunk.sequence_ = sequence;
    chunk.mark_ = kDataChunk;
    chunk.data_ = data;
    return chunk;
}

ClipboardChunk ClipboardChunk::end(ClipboardID id, std::uint32_t sequence)
{
    ClipboardChunk chunk;
    chunk.id_ = id;
    chunk.sequence_ = sequence;
    chunk.mark_ = kDataEnd;
    return chunk;
}

int ClipboardChunk::assemble(inputleap::IStream* stream, std::string& dataCached,
                             ClipboardID& id, std::uint32_t& sequence)
{
    std::uint8_t mark;
    std::string data;

    if (!ProtocolUtil::readf(stream, kMsgDClipboard + 4, &id, &sequence, &mark, &data)) {
        return kError;
    }

    if (mark == kDataStart) {
        s_expectedSize = inputleap::string::stringToSizeType(data);
        LOG((CLOG_DEBUG "start receiving clipboard data"));
        dataCached.clear();
        return kStart;
    }
    else if (mark == kDataChunk) {
        dataCached.append(data);
        return kNotFinish;
    }
    else if (mark == kDataEnd) {
        // validate
        if (id >= kClipboardEnd) {
            return kError;
        }
        else if (s_expectedSize != dataCached.size()) {
            LOG((CLOG_ERR "corrupted clipboard data, expected size=%d actual size=%d", s_expectedSize, dataCached.size()));
            return kError;
        }
        return kFinish;
    }

    LOG((CLOG_ERR "clipboard transmission failed: unknown error"));
    return kError;
}

} // namespace inputleap
