/*
 * barrier -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#pragma once

#include "net/ISocketMultiplexerJob.h"
#include "arch/Arch.h"

//! Use a method as a socket multiplexer job
/*!
A socket multiplexer job class that invokes a member function.
*/
class TSocketMultiplexerMethodJob : public ISocketMultiplexerJob {
public:
    using RunFunction = std::function<MultiplexerJobStatus(ISocketMultiplexerJob*, bool, bool, bool)>;

    //! run() invokes \c object->method(arg)
    TSocketMultiplexerMethodJob(const RunFunction& func,
                                ArchSocket socket, bool readable, bool writable) :
        func_{func},
        m_socket(ARCH->copySocket(socket)),
        m_readable(readable),
        m_writable(writable)
    {
    }

    ~TSocketMultiplexerMethodJob() override
    {
        ARCH->closeSocket(m_socket);
    }

    // IJob overrides
    virtual MultiplexerJobStatus run(bool readable, bool writable, bool error) override
    {
        if (func_) {
            return func_(this, readable, writable, error);
        }
        return {false, {}};
    }

    virtual ArchSocket getSocket() const override { return m_socket; }
    virtual bool isReadable() const override { return m_readable; }
    virtual bool isWritable() const override { return m_writable; }

private:
    RunFunction func_;
    ArchSocket            m_socket;
    bool                m_readable;
    bool                m_writable;
};


