/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2003 Chris Schoeneman
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

#include "inputleap/clipboard_types.h"
#include "inputleap/Fwd.h"
#include "base/Event.h"
#include "base/EventTypes.h"

namespace inputleap {

//! Screen interface
/*!
This interface defines the methods common to all screens.
*/
class IScreen {
public:
    virtual ~IScreen() { }

    struct ClipboardInfo {
    public:
        ClipboardID m_id;
        std::uint32_t m_sequenceNumber;
    };

    //! @name accessors
    //@{

    //! Get event target
    /*!
    Returns the target used for events created by this object.
    */
    virtual const void* get_event_target() const = 0;

    //! Get clipboard
    /*!
    Save the contents of the clipboard indicated by \c id and return
    true iff successful.
    */
    virtual bool getClipboard(ClipboardID id, IClipboard*) const = 0;

    //! Get screen shape
    /*!
    Return the position of the upper-left corner of the screen in \c x and
    \c y and the size of the screen in \c width and \c height.
    */
    virtual void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                          std::int32_t& height) const = 0;

    //! Get cursor position
    /*!
    Return the current position of the cursor in \c x and \c y.
    */
    virtual void getCursorPos(std::int32_t& x, std::int32_t& y) const = 0;

    //@}
};

} // namespace inputleap
