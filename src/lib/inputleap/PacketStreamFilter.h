/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "io/StreamFilter.h"
#include "io/StreamBuffer.h"

#include <mutex>

namespace inputleap {

class IEventQueue;

//! Packetizing stream filter
/*!
Filters a stream to read and write packets.
*/
class PacketStreamFilter : public StreamFilter {
public:
    PacketStreamFilter(IEventQueue* events, std::unique_ptr<IStream> stream);
    ~PacketStreamFilter() override;

    // IStream overrides
    virtual void close() override;
    virtual std::uint32_t read(void* buffer, std::uint32_t n) override;
    virtual void write(const void* buffer, std::uint32_t n) override;
    virtual void shutdownInput() override;
    virtual bool isReady() const override;
    virtual std::uint32_t getSize() const override;

protected:
    // StreamFilter overrides
    void filterEvent(const Event&) override;

private:
    bool isReadyNoLock() const;

    // returns false on erroneous packet size
    bool readPacketSize();
    bool readMore();

private:
    mutable std::mutex mutex_;
    std::uint32_t m_size;
    StreamBuffer m_buffer;
    bool m_inputShutdown;
    IEventQueue* m_events;
};

} // namespace inputleap
