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

class IEventQueue;

//! Packetizing stream filter
/*!
Filters a stream to read and write packets.
*/
class PacketStreamFilter : public StreamFilter {
public:
    PacketStreamFilter(IEventQueue* events, inputleap::IStream* stream, bool adoptStream = true);
    ~PacketStreamFilter() override;

    // IStream overrides
    virtual void close() override;
    virtual UInt32 read(void* buffer, UInt32 n) override;
    virtual void write(const void* buffer, UInt32 n) override;
    virtual void shutdownInput() override;
    virtual bool isReady() const override;
    virtual UInt32 getSize() const override;

protected:
    // StreamFilter overrides
    void filterEvent(const Event&) override;

private:
    bool                isReadyNoLock() const;

    // returns false on erroneous packet size
    bool readPacketSize();
    bool                readMore();

private:
    mutable std::mutex mutex_;
    UInt32                m_size;
    StreamBuffer        m_buffer;
    bool                m_inputShutdown;
    IEventQueue*        m_events;
};
