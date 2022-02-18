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

#include "inputleap/IClient.h"

namespace inputleap { class IStream; }

//! Generic proxy for client or primary
class BaseClientProxy : public IClient {
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
    void                setJumpCursorPos(SInt32 x, SInt32 y);

    //@}
    //! @name accessors
    //@{

    //! Get cursor position
    /*!
    Get the position of the cursor when last jumping from client.
    */
    void                getJumpCursorPos(SInt32& x, SInt32& y) const;

    //! Get cursor position
    /*!
    Return if this proxy is for client or primary.
    */
    virtual bool        isPrimary() const { return false; }

    //@}

    // IClient overrides
    virtual void sendDragInfo(std::uint32_t fileCount, const char* info, size_t size) = 0;
    virtual void        fileChunkSending(UInt8 mark, char* data, size_t dataSize) = 0;
    std::string getName() const override;
    virtual inputleap::IStream*
                        getStream() const = 0;

private:
    std::string m_name;
    SInt32                m_x, m_y;
};
