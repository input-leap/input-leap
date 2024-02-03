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

namespace inputleap {

// Event.h
class EventDataBase;
template<class T> class EventData;
class Event;

// EventQueue.h
class EventQueue;

// EventQueueTimer.h
class EventQueueTimer;

// EventTarget.h
class EventTarget;

// IEventQueue.h
class IEventQueue;

// IEventQueueBuffer.h
class IEventQueueBuffer;

// ILogOutputter.h
class ILogOutputter;

// Log.h
class Log;

// log_outputters.h
class StopLogOutputter;
class ConsoleLogOutputter;
class FileLogOutputter;
class SystemLogOutputter;
class SystemLogger;
class BufferedLogOutputter;
class MesssageBoxLogOutputter;

// SimpleEventQueueBuffer.h
class SimpleEventQueueBuffer;

// Stopwatch.h
class Stopwatch;

} // namespace inputleap
