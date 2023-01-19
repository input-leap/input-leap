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

#ifndef INPUTLEAP_LIB_BASE_EVENT_TARGET_H
#define INPUTLEAP_LIB_BASE_EVENT_TARGET_H

#include "Fwd.h"
#include "EventTypes.h"
#include <vector>

namespace inputleap {

/** EventTarget represents an object to which events are being sent. Event targets can be
    registered to IEventQueue. On destruction of an EventTarget, all such registrations are
    unregistered automatically.
*/
class EventTarget {
public:
    EventTarget();
    ~EventTarget();

    EventTarget(const EventTarget&) = delete;
    EventTarget& operator=(const EventTarget&) = delete;
private:
    friend class EventQueue;

    // Modifications to EventTarget data must be done from within EventQueue
    mutable IEventQueue* event_queue_ = nullptr;
};

} // namespace inputleap

#endif // INPUTLEAP_LIB_BASE_EVENT_TARGET_H
