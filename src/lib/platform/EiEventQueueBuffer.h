/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "config.h"

#include "base/IEventQueueBuffer.h"
#include "mt/Thread.h"
#include "platform/EiScreen.h"
#include "inputleap/IScreen.h"

#include <libei.h>

#include <queue>
#include <memory>
#include <mutex>

namespace inputleap {

//! Event queue buffer for Ei
class EiEventQueueBuffer : public IEventQueueBuffer {
public:
    EiEventQueueBuffer(EiScreen* screen, ei* ei, IEventQueue* events);
    ~EiEventQueueBuffer();

    void init() override { }
    void waitForEvent(double timeout_in_ms) override;
    Type getEvent(Event& event, uint32_t& dataID) override;
    bool addEvent(uint32_t dataID) override;
    bool isEmpty() const override;

private:
    ei* ei_;
    IEventQueue* events_;
    std::queue<std::pair<bool, uint32_t>> queue_;
    int pipe_w_, pipe_r_;

    mutable std::mutex mutex_;
};

} // namespace inputleap
