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
#include "base/IEventQueue.h"
#include "base/Event.h"
#include "base/PriorityQueue.h"
#include "base/Stopwatch.h"
#include "base/NonBlockingStream.h"

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
    void adoptBuffer(IEventQueueBuffer*) override;
    bool getEvent(Event& event, double timeout = -1.0) override;
    bool dispatchEvent(const Event& event) override;
    void addEvent(const Event& event) override;
    EventQueueTimer* newTimer(double duration, void* target) override;
    EventQueueTimer* newOneShotTimer(double duration, void* target) override;
    void deleteTimer(EventQueueTimer*) override;
    void adoptHandler(Event::Type type, void* target, IEventJob* handler) override;
    void removeHandler(Event::Type type, void* target) override;
    void removeHandlers(void* target) override;
    Event::Type registerTypeOnce(Event::Type& type, const char* name) override;
    IEventJob* getHandler(Event::Type type, void* target) const override;
    const char* getTypeName(Event::Type type) override;
    Event::Type getRegisteredType(const std::string& name) const override;
    void* getSystemTarget() override;
    void waitForReady() const override;

private:
    std::uint32_t saveEvent(const Event& event);
    Event removeEvent(std::uint32_t eventID);
    bool hasTimerExpired(Event& event);
    double getNextTimerTimeout() const;
    void addEventToBuffer(const Event& event);
    virtual bool parent_requests_shutdown() const;

private:
    class Timer {
    public:
        Timer(EventQueueTimer*, double timeout, double initialTime,
              void* target, bool oneShot);
        ~Timer();

        void reset();

        Timer& operator-=(double);
        operator double() const;

        bool isOneShot() const;
        EventQueueTimer* getTimer() const;
        void* getTarget() const;
        void fillEvent(TimerEvent&) const;

        bool operator<(const Timer&) const;

    private:
        EventQueueTimer* m_timer;
        double m_timeout;
        void* m_target;
        bool m_oneShot;
        double m_time;
    };

    typedef std::set<EventQueueTimer*> Timers;
    typedef PriorityQueue<Timer> TimerQueue;
    typedef std::map<std::uint32_t, Event> EventTable;
    typedef std::vector<std::uint32_t> EventIDList;
    typedef std::map<Event::Type, const char*> TypeMap;
    typedef std::map<std::string, Event::Type> NameMap;
    using TypeHandlerTable = std::map<Event::Type, std::unique_ptr<IEventJob>>;
    typedef std::map<void*, TypeHandlerTable> HandlerTable;

    int m_systemTarget;
    mutable std::mutex mutex_;

    // registered events
    Event::Type m_nextType;
    TypeMap m_typeMap;
    NameMap m_nameMap;

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

public:
    //
    // Event type providers.
    //
    ClientEvents& forClient() override;
    IStreamEvents& forIStream() override;
    IpcClientEvents& forIpcClient() override;
    IpcClientProxyEvents& forIpcClientProxy() override;
    IpcServerEvents& forIpcServer() override;
    IpcServerProxyEvents& forIpcServerProxy() override;
    IDataSocketEvents& forIDataSocket() override;
    IListenSocketEvents& forIListenSocket() override;
    ISocketEvents& forISocket() override;
    OSXScreenEvents& forOSXScreen() override;
    ClientListenerEvents& forClientListener() override;
    ClientProxyEvents& forClientProxy() override;
    ClientProxyUnknownEvents& forClientProxyUnknown() override;
    ServerEvents& forServer() override;
    ServerAppEvents& forServerApp() override;
    IKeyStateEvents& forIKeyState() override;
    IPrimaryScreenEvents& forIPrimaryScreen() override;
    IScreenEvents& forIScreen() override;
    ClipboardEvents& forClipboard() override;
    FileEvents& forFile() override;

private:
    ClientEvents* m_typesForClient;
    IStreamEvents* m_typesForIStream;
    IpcClientEvents* m_typesForIpcClient;
    IpcClientProxyEvents* m_typesForIpcClientProxy;
    IpcServerEvents* m_typesForIpcServer;
    IpcServerProxyEvents* m_typesForIpcServerProxy;
    IDataSocketEvents* m_typesForIDataSocket;
    IListenSocketEvents* m_typesForIListenSocket;
    ISocketEvents* m_typesForISocket;
    OSXScreenEvents* m_typesForOSXScreen;
    ClientListenerEvents* m_typesForClientListener;
    ClientProxyEvents* m_typesForClientProxy;
    ClientProxyUnknownEvents* m_typesForClientProxyUnknown;
    ServerEvents* m_typesForServer;
    ServerAppEvents* m_typesForServerApp;
    IKeyStateEvents* m_typesForIKeyState;
    IPrimaryScreenEvents* m_typesForIPrimaryScreen;
    IScreenEvents* m_typesForIScreen;
    ClipboardEvents* m_typesForClipboard;
    FileEvents* m_typesForFile;
    mutable std::mutex          ready_mutex_;
    mutable std::condition_variable ready_cv_;
    bool                        is_ready_ = false;
    std::queue<Event> m_pending;
    NonBlockingStream m_parentStream;
};

#define EVENT_TYPE_ACCESSOR(type_)                                            \
type_##Events&                                                                \
EventQueue::for##type_() {                                                \
    if (m_typesFor##type_ == nullptr) {                                        \
        m_typesFor##type_ = new type_##Events();                            \
        m_typesFor##type_->setEvents(dynamic_cast<IEventQueue*>(this));        \
    }                                                                        \
    return *m_typesFor##type_;                                                \
}

} // namespace inputleap
