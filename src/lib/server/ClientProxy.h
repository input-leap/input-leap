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

#pragma once

#include "server/BaseClientProxy.h"
#include "base/Event.h"
#include "base/EventTypes.h"
#include <memory>

namespace inputleap {

class IStream;

//! Generic proxy for client
class ClientProxy : public BaseClientProxy {
public:
    /*!
    \c name is the name of the client.
    */
    ClientProxy(const std::string& name, std::unique_ptr<IStream> stream);
    ~ClientProxy();

    //! @name manipulators
    //@{

    //! Disconnect
    /*!
    Ask the client to disconnect, using \p msg as the reason.
    */
    void close(const char* msg);

    //@}
    //! @name accessors
    //@{

    //! Get stream
    /*!
    Returns the original stream passed to the c'tor.
    */
    IStream* getStream() const override { return stream_.get(); }

    //@}

    // IScreen
    void* getEventTarget() const override;

private:
    std::unique_ptr<IStream> stream_;
};

} // namespace inputleap
