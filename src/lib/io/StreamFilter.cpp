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

#include "io/StreamFilter.h"
#include "base/IEventQueue.h"

namespace inputleap {

StreamFilter::StreamFilter(IEventQueue* events, std::unique_ptr<IStream> stream) :
    stream_(std::move(stream)),
    m_events(events)
{
    // replace handlers for m_stream
    m_events->removeHandlers(stream_->getEventTarget());
    m_events->add_handler(EventType::UNKNOWN, stream_->getEventTarget(),
                          [this](const auto& e){ handle_upstream_event(e); });
}

StreamFilter::~StreamFilter()
{
    m_events->removeHandler(EventType::UNKNOWN, stream_->getEventTarget());
}

void
StreamFilter::close()
{
    getStream()->close();
}

std::uint32_t StreamFilter::read(void* buffer, std::uint32_t n)
{
    return getStream()->read(buffer, n);
}

void StreamFilter::write(const void* buffer, std::uint32_t n)
{
    getStream()->write(buffer, n);
}

void
StreamFilter::flush()
{
    getStream()->flush();
}

void
StreamFilter::shutdownInput()
{
    getStream()->shutdownInput();
}

void
StreamFilter::shutdownOutput()
{
    getStream()->shutdownOutput();
}

void*
StreamFilter::getEventTarget() const
{
    return const_cast<void*>(static_cast<const void*>(this));
}

bool
StreamFilter::isReady() const
{
    return getStream()->isReady();
}

std::uint32_t StreamFilter::getSize() const
{
    return getStream()->getSize();
}

void
StreamFilter::filterEvent(const Event& event)
{
    Event copy{event.getType(), getEventTarget(), nullptr};
    copy.clone_data_from(event);
    m_events->dispatchEvent(copy);
}

void StreamFilter::handle_upstream_event(const Event& event)
{
    filterEvent(event);
}

} // namespace inputleap
