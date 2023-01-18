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

#include "inputleap/FileChunk.h"

#include "inputleap/ProtocolUtil.h"
#include "inputleap/protocol_types.h"
#include "io/IStream.h"
#include "base/Stopwatch.h"
#include "base/String.h"
#include "base/Log.h"

namespace inputleap {

static const std::uint16_t kIntervalThreshold = 1;

FileChunk FileChunk::start(std::size_t size)
{
    FileChunk chunk;
    chunk.mark_ = kDataStart;
    chunk.data_ = std::to_string(size);
    return chunk;
}

FileChunk FileChunk::data(std::uint8_t* data, size_t dataSize)
{
    FileChunk chunk;
    chunk.mark_ = kDataChunk;
    chunk.data_ = std::string(reinterpret_cast<char*>(data), dataSize);
    return chunk;
}

FileChunk FileChunk::end()
{
    FileChunk chunk;
    chunk.mark_ = kDataEnd;
    return chunk;
}

int FileChunk::assemble(inputleap::IStream* stream, std::string& dataReceived, size_t& expectedSize)
{
    // parse
    std::uint8_t mark = 0;
    std::string content;
    static size_t receivedDataSize;
    static double elapsedTime;
    static Stopwatch stopwatch;

    if (!ProtocolUtil::readf(stream, kMsgDFileTransfer + 4, &mark, &content)) {
        return kError;
    }

    switch (mark) {
    case kDataStart:
        dataReceived.clear();
        expectedSize = inputleap::string::stringToSizeType(content);
        receivedDataSize = 0;
        elapsedTime = 0;
        stopwatch.reset();

        if (CLOG->getFilter() >= kDEBUG2) {
            LOG((CLOG_DEBUG2 "recv file size=%s", content.c_str()));
            stopwatch.start();
        }
        return kStart;

    case kDataChunk:
        dataReceived.append(content);
        if (CLOG->getFilter() >= kDEBUG2) {
                LOG((CLOG_DEBUG2 "recv file chunk size=%i", content.size()));
                double interval = stopwatch.getTime();
                receivedDataSize += content.size();
                LOG((CLOG_DEBUG2 "recv file interval=%f s", interval));
                if (interval >= kIntervalThreshold) {
                    double averageSpeed = receivedDataSize / interval / 1000;
                    LOG((CLOG_DEBUG2 "recv file average speed=%f kb/s", averageSpeed));

                    receivedDataSize = 0;
                    elapsedTime += interval;
                    stopwatch.reset();
                }
            }
        return kNotFinish;

    case kDataEnd:
        if (expectedSize != dataReceived.size()) {
            LOG((CLOG_ERR "corrupted clipboard data, expected size=%d actual size=%d", expectedSize, dataReceived.size()));
            return kError;
        }

        if (CLOG->getFilter() >= kDEBUG2) {
            LOG((CLOG_DEBUG2 "file transfer finished"));
            elapsedTime += stopwatch.getTime();
            double averageSpeed = expectedSize / elapsedTime / 1000;
            LOG((CLOG_DEBUG2 "file transfer finished: total time consumed=%f s", elapsedTime));
            LOG((CLOG_DEBUG2 "file transfer finished: total data received=%i kb", expectedSize / 1000));
            LOG((CLOG_DEBUG2 "file transfer finished: total average speed=%f kb/s", averageSpeed));
        }
        return kFinish;
        default:
            break;
    }

    return kError;
}

void FileChunk::send(inputleap::IStream* stream, std::uint8_t mark, const std::string& data)
{
    switch (mark) {
    case kDataStart:
        LOG((CLOG_DEBUG2 "sending file chunk start: size=%s", data.c_str()));
        break;

    case kDataChunk:
        LOG((CLOG_DEBUG2 "sending file chunk: size=%i", data.size()));
        break;

    case kDataEnd:
        LOG((CLOG_DEBUG2 "sending file finished"));
        break;
    default:
        break;
    }

    ProtocolUtil::writef(stream, kMsgDFileTransfer, mark, &data);
}

} // namespace inputleap
