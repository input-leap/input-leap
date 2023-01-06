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

#ifndef INPUTLEAP_LIB_BASE_TIME_H
#define INPUTLEAP_LIB_BASE_TIME_H

namespace inputleap {

/*!
Blocks the calling thread for \c timeout seconds.  If
\c timeout < 0.0 then the call returns immediately.  If \c timeout
== 0.0 then the calling thread yields the CPU.

(cancellation point)
*/
void this_thread_sleep(double timeout);


//! Get the current time
/*!
Returns the number of seconds since some arbitrary starting time.
This should return as high a precision as reasonable.
*/
double current_time_seconds();

} // namespace inputleap

#endif // INPUTLEAP_LIB_NET_SECUREUTILS_H
