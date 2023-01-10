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
    Event(EventType type, void* target = nullptr, EventDataBase* data = nullptr,
          Flags flags = kNone) :
        type_{type},
        target_{target},
        data_{data},
        flags_{flags}
    {}

    /// Moves event data from another event
    void move_data_from(Event& other)
    {
        std::swap(data_, other.data_);
    }

    //! Release event data
    /*!
    Deletes event data for the given event (using free()).
    */
    static void deleteData(const Event& event)
    {
        if ((event.getFlags() & kDontFreeData) == 0) {
            delete event.data_;
            delete event.getDataObject();
        }
    }

    //! Set data (non-POD)
    /*!
    Set non-POD (non plain old data), where delete is called when the event
    is deleted, and the destructor is called.
    */
    void setDataObject(EventDataBase* dataObject)
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

    //! Get the event data (non-POD)
    /*!
    Returns the event data (non-POD). The difference between this and
    \c getData() is that when delete is called on this data, so non-POD
    (non plain old data) dtor is called.
    */
    EventDataBase* getDataObject() const { return data_object_; }

    //! Get event flags
    /*!
    Returns the event flags.
    */
    Flags getFlags() const { return flags_; }

private:
    EventType type_ = EventType::UNKNOWN;
    void* target_ = nullptr;
    EventDataBase* data_ = nullptr;
    Flags flags_ = 0;
    EventDataBase* data_object_ = nullptr;
};

} // namespace inputleap
