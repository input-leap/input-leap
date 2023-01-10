/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "IArchMultithread.h"

namespace inputleap {

bool IArchMultithread::wait_cond_var(std::condition_variable& cv,
                                     std::unique_lock<std::mutex>& lock, double timeout)
{
    // we can't wait on a condition variable and also wake it up for
    // cancellation since we don't use posix cancellation.  so we
    // must wake up periodically to check for cancellation.  we
    // can't simply go back to waiting after the check since the
    // condition may have changed and we'll have lost the signal.
    // so we have to return to the caller.  since the caller will
    // always check for spurious wakeups the only drawback here is
    // performance:  we're waking up a lot more than desired.
    static const double maxCancellationLatency = 0.1;
    if (timeout < 0.0 || timeout > maxCancellationLatency) {
        timeout = maxCancellationLatency;
    }

    // see if we should cancel this thread
    testCancelThread();

    auto ret = cv.wait_for(lock, seconds_to_chrono(timeout));

    // check for cancel again
    testCancelThread();

    if (ret == std::cv_status::no_timeout)
        return true;
    return false;
}

} // namespace inputleap
