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

#include "io/IStream.h"
#include "base/IEventQueue.h"
#include <memory>

namespace inputleap {

//! A stream filter
/*!
This class wraps a stream.  Subclasses provide indirect access
to the wrapped stream, typically performing some filtering.
*/
class StreamFilter : public IStream {
public:
    /*!
    Create a wrapper around \c stream.  Iff \c adoptStream is true then
    this object takes ownership of the stream and will delete it in the
    d'tor.
    */
    StreamFilter(IEventQueue* events, std::unique_ptr<IStream> stream);
    ~StreamFilter() override;

    // IStream overrides
    // These all just forward to the underlying stream except get_event_target.
    // Override as necessary.  get_event_target returns a pointer to this.
    void close() override;
    std::uint32_t read(void* buffer, std::uint32_t n) override;
    void write(const void* buffer, std::uint32_t n) override;
    void flush() override;
    void shutdownInput() override;
    void shutdownOutput() override;
    const void* get_event_target() const override;
    bool isReady() const override;
    std::uint32_t getSize() const override;

    //! Get the stream
    /*!
    Returns the stream passed to the c'tor.
    */
    inputleap::IStream* getStream() const { return stream_.get(); }

protected:
    //! Handle events from source stream
    /*!
    Does the event filtering.  The default simply dispatches an event
    identical except using this object as the event target.
    */
    virtual void filterEvent(const Event&);

private:
    void handle_upstream_event(const Event&);

private:
    std::unique_ptr<IStream> stream_;
    IEventQueue* m_events;
};

} // namespace inputleap
