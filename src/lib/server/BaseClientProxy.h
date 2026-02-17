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

#include "base/EventTarget.h"
#include "inputleap/Fwd.h"
#include "inputleap/IClient.h"
#include "inputleap/protocol_types.h"
#include <vector>

namespace inputleap {

class IClientConnection;
class IStream;

//! Generic proxy for client or primary
class BaseClientProxy : public IClient, public EventTarget {
public:
    /*!
    \c name is the name of the client.
    */
    BaseClientProxy(const std::string& name);
    ~BaseClientProxy();

    //! @name manipulators
    //@{

    //! Save cursor position
    /*!
    Save the position of the cursor when jumping from client.
    */
    void setJumpCursorPos(std::int32_t x, std::int32_t y);

    //@}
    //! @name accessors
    //@{

    //! Get cursor position
    /*!
    Get the position of the cursor when last jumping from client.
    */
    void getJumpCursorPos(std::int32_t& x, std::int32_t& y) const;

    //! Get cursor position
    /*!
    Return if this proxy is for client or primary.
    */
    virtual bool isPrimary() const { return false; }

    //@}

    //! Get per-monitor geometry
    virtual std::vector<MonitorInfo> getMonitors() const
    {
        std::int32_t x, y, w, h;
        getShape(x, y, w, h);
        MonitorInfo info;
        info.m_x = x; info.m_y = y; info.m_w = w; info.m_h = h; info.m_index = 0;
        return { info };
    }

    // IClient overrides
    virtual void sendDragInfo(std::uint32_t fileCount, const char* info, size_t size) = 0;
    virtual void file_chunk_sending(const FileChunk& chunk) = 0;
    std::string getName() const override;
    virtual IClientConnection& get_conn() const = 0;

private:
    std::string m_name;
    std::int32_t m_x, m_y;
};

} // namespace inputleap
