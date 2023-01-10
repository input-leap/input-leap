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

#include "EventTypes.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <map>

namespace inputleap {

class EventData {
public:
    EventData() { }
    virtual ~EventData() { }
};

/// Event holds an event type and a pointer to event data. It is movable, but not copyable
class Event {
public:
    typedef std::uint32_t Flags;
    enum {
        kNone                = 0x00,    //!< No flags
        kDeliverImmediately  = 0x01,    //!< Dispatch and free event immediately
        kDontFreeData        = 0x02    //!< Don't free data in deleteData
    };

    Event() = default;

    Event(const Event&) = delete;
    Event(Event&& other) = default;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = default;

    //! Create \c Event with data (POD)
    /*!
    The \p data must be POD (plain old data) allocated by malloc(),
    which means it cannot have a constructor, destructor or be
    composed of any types that do. For non-POD (normal C++ objects
    use \c setDataObject().
    \p target is the intended recipient of the event.
    \p flags is any combination of \c Flags.
    */
    Event(EventType type, void* target = nullptr, void* data = nullptr, Flags flags = kNone) :
        type_{type},
        target_{target},
        data_{data},
        flags_{flags}
    {}

    //! Release event data
    /*!
    Deletes event data for the given event (using free()).
    */
    static void deleteData(const Event& event)
    {
        switch (event.getType()) {
        case EventType::UNKNOWN:
        case EventType::QUIT:
        case EventType::SYSTEM:
        case EventType::TIMER:
            break;

        default:
            if ((event.getFlags() & kDontFreeData) == 0) {
                std::free(event.getData());
                delete event.getDataObject();
            }
            break;
        }
    }

    //! Set data (non-POD)
    /*!
    Set non-POD (non plain old data), where delete is called when the event
    is deleted, and the destructor is called.
    */
    void setDataObject(EventData* dataObject)
    {
        assert(data_object_ == nullptr);
        data_object_ = dataObject;
    }


    //! Get event type
    /*!
    Returns the event type.
    */
    EventType getType() const { return type_; }

    //! Get the event target
    /*!
    Returns the event target.
    */
    void* getTarget() const { return target_; }

    //! Get the event data (POD).
    /*!
    Returns the event data (POD).
    */
    void* getData() const { return data_; }

    //! Get the event data (non-POD)
    /*!
    Returns the event data (non-POD). The difference between this and
    \c getData() is that when delete is called on this data, so non-POD
    (non plain old data) dtor is called.
    */
    EventData* getDataObject() const { return data_object_; }

    //! Get event flags
    /*!
    Returns the event flags.
    */
    Flags getFlags() const { return flags_; }

private:
    EventType type_ = EventType::UNKNOWN;
    void* target_ = nullptr;
    void* data_ = nullptr;
    Flags flags_ = 0;
    EventData* data_object_ = nullptr;
};

} // namespace inputleap
