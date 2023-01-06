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

#include "inputleap/PacketStreamFilter.h"
#include "inputleap/protocol_types.h"
#include "base/IEventQueue.h"
#include "base/TMethodEventJob.h"

#include <cstring>
#include <memory>

//
// PacketStreamFilter
//

PacketStreamFilter::PacketStreamFilter(IEventQueue* events, inputleap::IStream* stream, bool adoptStream) :
    StreamFilter(events, stream, adoptStream),
    m_size(0),
    m_inputShutdown(false),
    m_events(events)
{
    // do nothing
}

PacketStreamFilter::~PacketStreamFilter()
{
    // do nothing
}

void
PacketStreamFilter::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    m_size = 0;
    m_buffer.pop(m_buffer.getSize());
    StreamFilter::close();
}

std::uint32_t PacketStreamFilter::read(void* buffer, std::uint32_t n)
{
    if (n == 0) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // if not enough data yet then give up
    if (!isReadyNoLock()) {
        return 0;
    }

    // read no more than what's left in the buffered packet
    if (n > m_size) {
        n = m_size;
    }

    // read it
    if (buffer != nullptr) {
        memcpy(buffer, m_buffer.peek(n), n);
    }
    m_buffer.pop(n);
    m_size -= n;

    // get next packet's size if we've finished with this packet and
    // there's enough data to do so.
    readPacketSize();

    if (m_inputShutdown && m_size == 0) {
        m_events->addEvent(Event(m_events->forIStream().inputShutdown(),
                                 getEventTarget(), nullptr));
    }

    return n;
}

void PacketStreamFilter::write(const void* buffer, std::uint32_t count)
{
    // write the length of the payload
    std::uint8_t length[4];
    length[0] = static_cast<std::uint8_t>((count >> 24) & 0xff);
    length[1] = static_cast<std::uint8_t>((count >> 16) & 0xff);
    length[2] = static_cast<std::uint8_t>((count >> 8) & 0xff);
    length[3] = static_cast<std::uint8_t>(count& 0xff);
    getStream()->write(length, sizeof(length));

    // write the payload
    getStream()->write(buffer, count);
}

void
PacketStreamFilter::shutdownInput()
{
    std::lock_guard<std::mutex> lock(mutex_);
    m_size = 0;
    m_buffer.pop(m_buffer.getSize());
    StreamFilter::shutdownInput();
}

bool
PacketStreamFilter::isReady() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return isReadyNoLock();
}

std::uint32_t PacketStreamFilter::getSize() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return isReadyNoLock() ? m_size : 0;
}

bool
PacketStreamFilter::isReadyNoLock() const
{
    return (m_size != 0 && m_buffer.getSize() >= m_size);
}

bool PacketStreamFilter::readPacketSize()
{
    // note -- mutex_ must be locked on entry

    if (m_size == 0 && m_buffer.getSize() >= 4) {
        std::uint8_t buffer[4];
        memcpy(buffer, m_buffer.peek(sizeof(buffer)), sizeof(buffer));
        m_buffer.pop(sizeof(buffer));
        m_size = (static_cast<std::uint32_t>(buffer[0]) << 24) |
                 (static_cast<std::uint32_t>(buffer[1]) << 16) |
                 (static_cast<std::uint32_t>(buffer[2]) <<  8) |
                  static_cast<std::uint32_t>(buffer[3]);

        if (m_size > PROTOCOL_MAX_MESSAGE_LENGTH) {
            m_events->addEvent(Event(m_events->forIStream().inputFormatError(), getEventTarget()));
            return false;
        }
    }
    return true;
}

bool
PacketStreamFilter::readMore()
{
    // note if we have whole packet
    bool wasReady = isReadyNoLock();

    // read more data
    char buffer[4096];
    std::uint32_t n = getStream()->read(buffer, sizeof(buffer));
    while (n > 0) {
        m_buffer.write(buffer, n);

        // if we don't yet have the next packet size then get it, if possible.
        // Note that we can't wait for whole pending data to arrive because it may be huge in
        // case of malicious or erroneous peer.
        if (!readPacketSize()) {
            break;
        }

        n = getStream()->read(buffer, sizeof(buffer));
    }

    // note if we now have a whole packet
    bool isReady = isReadyNoLock();

    // if we weren't ready before but now we are then send a
    // input ready event apparently from the filtered stream.
    return (wasReady != isReady);
}

void
PacketStreamFilter::filterEvent(const Event& event)
{
    if (event.getType() == m_events->forIStream().inputReady()) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!readMore()) {
            return;
        }
    }
    else if (event.getType() == m_events->forIStream().inputShutdown()) {
        // discard this if we have buffered data
        std::lock_guard<std::mutex> lock(mutex_);
        m_inputShutdown = true;
        if (m_size != 0) {
            return;
        }
    }

    // pass event
    StreamFilter::filterEvent(event);
}
