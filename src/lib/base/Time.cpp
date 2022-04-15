/*
    InputLeap -- mouse and keyboard sharing utility
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

#include "Time.h"
#include "arch/Arch.h"
#include <chrono>
#include <thread>

namespace inputleap {

void this_thread_sleep(double timeout_seconds)
{
    ARCH->testCancelThread();
    if (timeout_seconds < 0.0) {
        return;
    }

    auto milliseconds = static_cast<std::uint64_t>(timeout_seconds * 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

double current_time_seconds()
{
    auto since_epoch = std::chrono::steady_clock::now().time_since_epoch();
    auto us_since_epoch = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch).count();
    return us_since_epoch / 1000000.0;
}

} // namespace inputleap
