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

#include "arch/IArchMultithread.h"
#include "EventTarget.h"
#include "base/IEventQueue.h"
#include "base/Event.h"
#include "base/PriorityQueue.h"
#include "base/Stopwatch.h"

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>

namespace inputleap {

//! Event queue
/*!
An event queue that implements the platform independent parts and
delegates the platform dependent parts to a subclass.
*/
class EventQueue : public IEventQueue {
public:
    EventQueue();
    ~EventQueue() override;

    // IEventQueue overrides
    void loop() override;
    void set_buffer(std::unique_ptr<IEventQueueBuffer> buffer) override;
    bool getEvent(Event& event, double timeout = -1.0) override;
    bool dispatchEvent(const Event& event) override;
    void add_event(Event&& event) override;
    EventQueueTimer* newTimer(double duration, const EventTarget* target) override;
    EventQueueTimer* newOneShotTimer(double duration, const EventTarget* target) override;
    void deleteTimer(EventQueueTimer*) override;
    void add_handler(EventType type, const EventTarget* target,
                     const EventHandler& handler) override;
    void remove_handler(EventType type, const EventTarget* target) override;
    void remove_handlers(const EventTarget* target) override;
    const EventTarget* getSystemTarget() override;
    void waitForReady() const override;

private:
    std::uint32_t save_event(Event&& event);
    Event removeEvent(std::uint32_t eventID);
    bool hasTimerExpired(Event& event);
    double getNextTimerTimeout() const;
    void add_event_to_buffer(Event&& event);

private:
    class Timer {
    public:
        Timer(EventQueueTimer*, double timeout, double initialTime,
              const EventTarget* target, bool oneShot);
        ~Timer();

        void reset();

        Timer& operator-=(double);
        operator double() const;

        bool isOneShot() const;
        EventQueueTimer* getTimer() const;
        const EventTarget* get_target() const { return target_; }
        void fillEvent(TimerEvent&) const;

        bool operator<(const Timer&) const;

    private:
        EventQueueTimer* m_timer;
        double m_timeout;
        const EventTarget* target_;
        bool m_oneShot;
        double m_time;
    };

    typedef std::set<EventQueueTimer*> Timers;
    typedef PriorityQueue<Timer> TimerQueue;
    typedef std::map<std::uint32_t, Event> EventTable;
    typedef std::vector<std::uint32_t> EventIDList;
    using TypeHandlerTable = std::map<EventType, std::shared_ptr<EventHandler>>;
    using HandlerTable = std::map<const EventTarget*, TypeHandlerTable>;

    EventTarget system_target_;
    mutable std::mutex mutex_;

    // buffer of events
    std::unique_ptr<IEventQueueBuffer> buffer_;

    // saved events
    EventTable m_events;
    EventIDList m_oldEventIDs;

    // timers
    Stopwatch m_time;
    Timers m_timers;
    TimerQueue m_timerQueue;
    TimerEvent m_timerEvent;

    // event handlers
    HandlerTable m_handlers;

private:
    // returns nullptr if handler is not found

    std::shared_ptr<EventHandler> get_handler(EventType type, const EventTarget* target) const;

    mutable std::mutex          ready_mutex_;
    mutable std::condition_variable ready_cv_;
    bool                        is_ready_ = false;
    std::queue<Event> m_pending;
};

} // namespace inputleap
