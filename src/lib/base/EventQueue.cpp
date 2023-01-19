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

#include "base/EventQueue.h"
#include "EventQueueTimer.h"

#include "arch/Arch.h"
#include "base/SimpleEventQueueBuffer.h"
#include "base/Stopwatch.h"
#include "base/EventTypes.h"
#include "base/Log.h"
#include "base/XBase.h"

namespace inputleap {

// interrupt handler.  this just adds a quit event to the queue.
static
void
interrupt(Arch::ESignal, void* data)
{
    EventQueue* events = static_cast<EventQueue*>(data);
    events->add_event(EventType::QUIT);
}

EventQueue::EventQueue()
{
    ARCH->setSignalHandler(Arch::kINTERRUPT, &interrupt, this);
    ARCH->setSignalHandler(Arch::kTERMINATE, &interrupt, this);
    buffer_ = std::make_unique<SimpleEventQueueBuffer>();
}

EventQueue::~EventQueue()
{
    ARCH->setSignalHandler(Arch::kINTERRUPT, nullptr, nullptr);
    ARCH->setSignalHandler(Arch::kTERMINATE, nullptr, nullptr);

    for (const auto& handlers : m_handlers) {
        handlers.first->event_queue_ = nullptr;
    }
}

void
EventQueue::loop()
{
    buffer_->init();
    {
        std::unique_lock<std::mutex> lock(ready_mutex_);
        is_ready_ = true;
        ready_cv_.notify_one();
    }
    LOG((CLOG_DEBUG "event queue is ready"));
    while (!m_pending.empty()) {
        LOG((CLOG_DEBUG "add pending events to buffer"));
        Event& event = m_pending.front();
        add_event_to_buffer(std::move(event));
        m_pending.pop();
    }

    Event event;
    getEvent(event);
    while (event.getType() != EventType::QUIT) {
        dispatchEvent(event);
        Event::deleteData(event);
        getEvent(event);
    }
}

void
EventQueue::adoptBuffer(IEventQueueBuffer* buffer)
{
    std::lock_guard<std::mutex> lock(mutex_);

    LOG((CLOG_DEBUG "adopting new buffer"));

    if (m_events.size() != 0) {
        // this can come as a nasty surprise to programmers expecting
        // their events to be raised, only to have them deleted.
        LOG((CLOG_DEBUG "discarding %d event(s)", m_events.size()));
    }

    // discard old buffer and old events
    buffer_.reset();
    for (EventTable::iterator i = m_events.begin(); i != m_events.end(); ++i) {
        Event::deleteData(i->second);
    }
    m_events.clear();
    m_oldEventIDs.clear();

    // use new buffer
    buffer_.reset(buffer);
    if (buffer_ == nullptr) {
        buffer_ = std::make_unique<SimpleEventQueueBuffer>();
    }
}

bool
EventQueue::getEvent(Event& event, double timeout)
{
    Stopwatch timer(true);
retry:
    // if no events are waiting then handle timers and then wait
    while (buffer_->isEmpty()) {
        // handle timers first
        if (hasTimerExpired(event)) {
            return true;
        }

        // get time remaining in timeout
        double timeLeft = timeout - timer.getTime();
        if (timeout >= 0.0 && timeLeft <= 0.0) {
            return false;
        }

        // get time until next timer expires.  if there is a timer
        // and it'll expire before the client's timeout then use
        // that duration for our timeout instead.
        double timerTimeout = getNextTimerTimeout();
        if (timeout < 0.0 || (timerTimeout >= 0.0 && timerTimeout < timeLeft)) {
            timeLeft = timerTimeout;
        }

        // wait for an event
        buffer_->waitForEvent(timeLeft);
    }

    // get the event
    std::uint32_t dataID;
    IEventQueueBuffer::Type type = buffer_->getEvent(event, dataID);
    switch (type) {
    case IEventQueueBuffer::kNone:
        if (timeout < 0.0 || timeout <= timer.getTime()) {
            // don't want to fail if client isn't expecting that
            // so if getEvent() fails with an infinite timeout
            // then just try getting another event.
            goto retry;
        }
        return false;

    case IEventQueueBuffer::kSystem:
        return true;

    case IEventQueueBuffer::kUser:
        {
            std::lock_guard<std::mutex> lock(mutex_);
            event = removeEvent(dataID);
            return true;
        }

    default:
        assert(0 && "invalid event type");
        return false;
    }
}

bool
EventQueue::dispatchEvent(const Event& event)
{
    auto* target = event.getTarget();

    auto type_handler = get_handler(event.getType(), target);
    if (type_handler) {
        (*type_handler)(event);
        return true;
    }

    auto any_handler = get_handler(EventType::UNKNOWN, target);
    if (any_handler) {
        (*any_handler)(event);
        return true;
    }
    return false;
}

void EventQueue::add_event(Event&& event)
{
    // discard bogus event types
    switch (event.getType()) {
    case EventType::UNKNOWN:
    case EventType::SYSTEM:
    case EventType::TIMER:
        return;

    default:
        break;
    }

    if ((event.getFlags() & Event::kDeliverImmediately) != 0) {
        dispatchEvent(event);
        Event::deleteData(event);
    }
    else if (!is_ready_) {
        m_pending.push(std::move(event));
    } else {
        add_event_to_buffer(std::move(event));
    }
}

void EventQueue::add_event_to_buffer(Event&& event)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // store the event's data locally
    std::uint32_t eventID = save_event(std::move(event));

    // add it
    if (!buffer_->addEvent(eventID)) {
        // failed to send event
        auto removed_event = removeEvent(eventID);
        Event::deleteData(removed_event);
    }
}

EventQueueTimer* EventQueue::newTimer(double duration, const EventTarget* target)
{
    assert(duration > 0.0);

    EventQueueTimer* timer = buffer_->newTimer(duration, false);
    if (target == nullptr) {
        target = timer;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    m_timers.insert(timer);
    // initial duration is requested duration plus whatever's on
    // the clock currently because the latter will be subtracted
    // the next time we check for timers.
    m_timerQueue.push(Timer(timer, duration,
                            duration + m_time.getTime(), target, false));
    return timer;
}

EventQueueTimer* EventQueue::newOneShotTimer(double duration, const EventTarget* target)
{
    assert(duration > 0.0);

    EventQueueTimer* timer = buffer_->newTimer(duration, true);
    if (target == nullptr) {
        target = timer;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    m_timers.insert(timer);
    // initial duration is requested duration plus whatever's on
    // the clock currently because the latter will be subtracted
    // the next time we check for timers.
    m_timerQueue.push(Timer(timer, duration,
                            duration + m_time.getTime(), target, true));
    return timer;
}

void
EventQueue::deleteTimer(EventQueueTimer* timer)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (TimerQueue::iterator index = m_timerQueue.begin();
                            index != m_timerQueue.end(); ++index) {
        if (index->getTimer() == timer) {
            m_timerQueue.erase(index);
            break;
        }
    }
    Timers::iterator index = m_timers.find(timer);
    if (index != m_timers.end()) {
        m_timers.erase(index);
    }
    buffer_->deleteTimer(timer);
}

void EventQueue::add_handler(EventType type, const EventTarget* target, const EventHandler& handler)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (target->event_queue_ == nullptr) {
        target->event_queue_ = this;
    } else if (target->event_queue_ != this) {
        throw std::invalid_argument("EventTarget added to wrong EventQueue");
    }

    m_handlers[target][type] = std::make_shared<EventHandler>(handler);
}

void EventQueue::remove_handler(EventType type, const EventTarget* target)
{
    if (target->event_queue_ == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    HandlerTable::iterator index = m_handlers.find(target);
    if (index != m_handlers.end()) {
        TypeHandlerTable& typeHandlers = index->second;
        TypeHandlerTable::iterator index2 = typeHandlers.find(type);
        if (index2 != typeHandlers.end()) {
            typeHandlers.erase(index2);
        }
        if (typeHandlers.empty()) {
            m_handlers.erase(index);
            target->event_queue_ = nullptr;
        }
    }
}

void EventQueue::remove_handlers(const EventTarget* target)
{
    if (target->event_queue_ == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (target->event_queue_ != this) {
        throw std::invalid_argument("EventTarget sent to wrong EventQueue");
    }

    HandlerTable::iterator index = m_handlers.find(target);
    if (index != m_handlers.end()) {
        m_handlers.erase(index);
    }
    target->event_queue_ = nullptr;
}

std::shared_ptr<EventQueue::EventHandler>
    EventQueue::get_handler(EventType type, const EventTarget* target) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    HandlerTable::const_iterator index = m_handlers.find(target);
    if (index != m_handlers.end()) {
        const TypeHandlerTable& typeHandlers = index->second;
        TypeHandlerTable::const_iterator index2 = typeHandlers.find(type);
        if (index2 != typeHandlers.end()) {
            return index2->second;
        }
    }
    return nullptr;
}

std::uint32_t EventQueue::save_event(Event&& event)
{
    // choose id
    std::uint32_t id;
    if (!m_oldEventIDs.empty()) {
        // reuse an id
        id = m_oldEventIDs.back();
        m_oldEventIDs.pop_back();
    }
    else {
        // make a new id
        id = static_cast<std::uint32_t>(m_events.size());
    }

    // save data
    m_events[id] = std::move(event);
    return id;
}

Event EventQueue::removeEvent(std::uint32_t eventID)
{
    // look up id
    EventTable::iterator index = m_events.find(eventID);
    if (index == m_events.end()) {
        return Event();
    }

    // get data
    Event event = std::move(index->second);
    m_events.erase(index);

    // save old id for reuse
    m_oldEventIDs.push_back(eventID);

    return event;
}

bool
EventQueue::hasTimerExpired(Event& event)
{
    // return true if there's a timer in the timer priority queue that
    // has expired.  if returning true then fill in event appropriately
    // and reset and reinsert the timer.
    if (m_timerQueue.empty()) {
        return false;
    }

    // get time elapsed since last check
    const double time = m_time.getTime();
    m_time.reset();

    // countdown elapsed time
    for (TimerQueue::iterator index = m_timerQueue.begin();
                            index != m_timerQueue.end(); ++index) {
        (*index) -= time;
    }

    // done if no timers are expired
    if (m_timerQueue.top() > 0.0) {
        return false;
    }

    // remove timer from queue
    Timer timer = m_timerQueue.top();
    m_timerQueue.pop();

    // prepare event and reset the timer's clock
    timer.fillEvent(m_timerEvent);
    event = Event(EventType::TIMER, timer.get_target(),
                  create_event_data<TimerEvent*>(&m_timerEvent));
    timer.reset();

    // reinsert timer into queue if it's not a one-shot
    if (!timer.isOneShot()) {
        m_timerQueue.push(timer);
    }

    return true;
}

double
EventQueue::getNextTimerTimeout() const
{
    // return -1 if no timers, 0 if the top timer has expired, otherwise
    // the time until the top timer in the timer priority queue will
    // expire.
    if (m_timerQueue.empty()) {
        return -1.0;
    }
    if (m_timerQueue.top() <= 0.0) {
        return 0.0;
    }
    return m_timerQueue.top();
}

const EventTarget* EventQueue::getSystemTarget()
{
    return &system_target_;
}

void
EventQueue::waitForReady() const
{
    std::unique_lock<std::mutex> lock(ready_mutex_);

    if (!ready_cv_.wait_for(lock, std::chrono::seconds{10}, [this](){ return is_ready_; })) {
        throw std::runtime_error("event queue is not ready within 5 sec");
    }
}

//
// EventQueue::Timer
//

EventQueue::Timer::Timer(EventQueueTimer* timer, double timeout,
                         double initialTime, const EventTarget* target, bool oneShot) :
    m_timer(timer),
    m_timeout(timeout),
    target_(target),
    m_oneShot(oneShot),
    m_time(initialTime)
{
    assert(m_timeout > 0.0);
}

EventQueue::Timer::~Timer()
{
    // do nothing
}

void
EventQueue::Timer::reset()
{
    m_time = m_timeout;
}

EventQueue::Timer&
EventQueue::Timer::operator-=(double dt)
{
    m_time -= dt;
    return *this;
}

EventQueue::Timer::operator double() const
{
    return m_time;
}

bool
EventQueue::Timer::isOneShot() const
{
    return m_oneShot;
}

EventQueueTimer*
EventQueue::Timer::getTimer() const
{
    return m_timer;
}

void
EventQueue::Timer::fillEvent(TimerEvent& event) const
{
    event.m_timer = m_timer;
    event.m_count = 0;
    if (m_time <= 0.0) {
        event.m_count = static_cast<std::uint32_t>((m_timeout - m_time) / m_timeout);
    }
}

bool
EventQueue::Timer::operator<(const Timer& t) const
{
    return m_time < t.m_time;
}

} // namespace inputleap
