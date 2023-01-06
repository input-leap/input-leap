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

#pragma once

#include "base/EventQueue.h"

class EventQueueTimer {};

class TestEventQueue : public EventQueue {
public:
    TestEventQueue() : m_quitTimeoutTimer(nullptr) { }

    void handleQuitTimeout(const Event&, void* vclient);
    void raiseQuitEvent();
    void initQuitTimeout(double timeout);
    void cleanupQuitTimeout();
    bool parent_requests_shutdown() const override;

private:
    void timeoutThread(void*);

private:
    EventQueueTimer* m_quitTimeoutTimer;
};
