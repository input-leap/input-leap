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

#include "Fwd.h"
#include "EventTypes.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <stdexcept>

namespace inputleap {

class EventDataBase {
public:
    virtual EventDataBase* clone() const = 0;
    virtual ~EventDataBase() { }
};

template<class T>
class EventData : public EventDataBase {
public:
    EventData(const T& data) : data_{data} {}
    EventData(T&& data) : data_{std::move(data)} {}
    ~EventData() = default;

    EventData<T>* clone() const override { return new EventData<T>(*this); }

    T& data() { return data_; }
    const T& data() const { return data_; }
private:
    T data_;
};

template<class T, class U>
EventData<T>* create_event_data(U&& data)
{
    return new EventData<T>(std::forward<U>(data));
}

/// Event holds an event type and a pointer to event data. It is movable, but not copyable
class Event {
public:
    typedef std::uint32_t Flags;
    enum {
        kNone                = 0x00,    //!< No flags
        kDeliverImmediately  = 0x01,    //!< Dispatch and free event immediately
    };

    Event() = default;

    Event(const Event&) = delete;
    Event(Event&& other) = default;
    Event& operator=(const Event&) = delete;
    Event& operator=(Event&&) = default;

    /** Create event with data
        @param target is the intended recipient of the event.
        @param flags is any combination of \c Flags.
    */
    Event(EventType type, const EventTarget* target = nullptr, EventDataBase* data = nullptr,
          Flags flags = kNone) :
        type_{type},
        target_{target},
        data_{data},
        flags_{flags}
    {}

    /// Moves event data from another event
    void clone_data_from(const Event& other)
    {
        if (data_ != nullptr) {
            throw std::invalid_argument("data must be null to clone it from other event");
        }
        if (other.data_ == nullptr) {
            return;
        }
        data_ = other.data_->clone();
    }

    //! Release event data
    /*!
    Deletes event data for the given event (using free()).
    */
    static void deleteData(const Event& event)
    {
        delete event.data_;
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
    const EventTarget* getTarget() const { return target_; }

    /// Returns stored event data as specified type
    template<class T>
    const T& get_data_as() const
    {
        if (data_ == nullptr) {
            throw std::runtime_error("Data does not exist");
        }
        return static_cast<const EventData<T>*>(data_)->data();
    }

    template<class T>
    T& get_data_as()
    {
        if (data_ == nullptr) {
            throw std::runtime_error("Data does not exist");
        }
        return static_cast<EventData<T>*>(data_)->data();
    }

    //! Get event flags
    /*!
    Returns the event flags.
    */
    Flags getFlags() const { return flags_; }

private:
    EventType type_ = EventType::UNKNOWN;
    const EventTarget* target_ = nullptr;
    EventDataBase* data_ = nullptr;
    Flags flags_ = 0;
    EventDataBase* data_object_ = nullptr;
};

} // namespace inputleap
