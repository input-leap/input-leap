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

#include "base/FunctionEventJob.h"

//
// FunctionEventJob
//

FunctionEventJob::FunctionEventJob(
                void (*func)(const Event&, void*), void* arg) :
    m_func(func),
    m_arg(arg)
{
    // do nothing
}

FunctionEventJob::~FunctionEventJob()
{
    // do nothing
}

void
FunctionEventJob::run(const Event& event)
{
    if (m_func != nullptr) {
        m_func(event, m_arg);
    }
}
