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

#include "inputleap/mouse_types.h"
#include "base/Event.h"
#include "base/EventTypes.h"

//! Secondary screen interface
/*!
This interface defines the methods common to all platform dependent
secondary screen implementations.
*/
class ISecondaryScreen {
public:
    virtual ~ISecondaryScreen() { }

    //! @name accessors
    //@{

    //! Fake mouse press/release
    /*!
    Synthesize a press or release of mouse button \c id.
    */
    virtual void fakeMouseButton(ButtonID id, bool press) = 0;

    //! Fake mouse move
    /*!
    Synthesize a mouse move to the absolute coordinates \c x,y.
    */
    virtual void fakeMouseMove(std::int32_t x, std::int32_t y) = 0;

    //! Fake mouse move
    /*!
    Synthesize a mouse move to the relative coordinates \c dx,dy.
    */
    virtual void fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const = 0;

    //! Fake mouse wheel
    /*!
    Synthesize a mouse wheel event of amount \c xDelta and \c yDelta.
    */
    virtual void fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const = 0;

    //@}
};
