/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#include "test/global/TestEventQueue.h"

#include "base/EventQueueTimer.h"
#include "base/Log.h"
#include "base/SimpleEventQueueBuffer.h"
#include <stdexcept>

namespace inputleap {

void
TestEventQueue::raiseQuitEvent()
{
    add_event(EventType::QUIT);
}

void
TestEventQueue::initQuitTimeout(double timeout)
{
    assert(m_quitTimeoutTimer == nullptr);
    m_quitTimeoutTimer = newOneShotTimer(timeout, nullptr);
    add_handler(EventType::TIMER, m_quitTimeoutTimer,
                [](const auto& e){ throw std::runtime_error("test event queue timeout"); });
}

void
TestEventQueue::cleanupQuitTimeout()
{
    removeHandler(EventType::TIMER, m_quitTimeoutTimer);
    delete m_quitTimeoutTimer;
    m_quitTimeoutTimer = nullptr;
}

} // namespace inputleap
