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

#include "base/IEventJob.h"
#include <cstddef>

//! Use a function as an event job
/*!
An event job class that invokes a function.
*/
class FunctionEventJob : public IEventJob {
public:
    //! run() invokes \c func(arg)
    FunctionEventJob(void (*func)(const Event&, void*), void* arg = nullptr);
    ~FunctionEventJob() override;

    // IEventJob overrides
    void run(const Event&) override;

private:
    void  (*m_func)(const Event&, void*);
    void* m_arg;
};
