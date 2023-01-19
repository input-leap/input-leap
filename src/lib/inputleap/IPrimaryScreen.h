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

#include "inputleap/key_types.h"
#include "inputleap/mouse_types.h"
#include "base/Event.h"
#include "base/EventTypes.h"

namespace inputleap {

//! Primary screen interface
/*!
This interface defines the methods common to all platform dependent
primary screen implementations.
*/
class IPrimaryScreen {
public:
    virtual ~IPrimaryScreen() { }

    //! Button event data
    class ButtonInfo {
    public:
        ButtonInfo(ButtonID button, KeyModifierMask mask) :
            m_button{button},
            m_mask{mask}
        {}

        static bool equal(const ButtonInfo*, const ButtonInfo*);

    public:
        ButtonID m_button;
        KeyModifierMask m_mask;
    };
    //! Motion event data
    class MotionInfo {
    public:
        MotionInfo(std::int32_t x, std::int32_t y) :
            m_x{x},
            m_y{y}
        {}

    public:
        std::int32_t m_x;
        std::int32_t m_y;
    };
    //! Wheel motion event data
    class WheelInfo {
    public:
        WheelInfo(std::int32_t x_delta, std::int32_t y_delta) :
            m_xDelta{x_delta},
            m_yDelta{y_delta}
        {}

    public:
        std::int32_t m_xDelta;
        std::int32_t m_yDelta;
    };
    //! Hot key event data
    class HotKeyInfo {
    public:
        HotKeyInfo(std::uint32_t id) : m_id{id} {}

    public:
        std::uint32_t m_id;
    };

    //! @name manipulators
    //@{

    //! Update configuration
    /*!
    This is called when the configuration has changed.  \c activeSides
    is a bitmask of EDirectionMask indicating which sides of the
    primary screen are linked to clients.  Override to handle the
    possible change in jump zones.
    */
    virtual void reconfigure(std::uint32_t activeSides) = 0;

    //! Warp cursor
    /*!
    Warp the cursor to the absolute coordinates \c x,y.  Also
    discard input events up to and including the warp before
    returning.
    */
    virtual void warpCursor(std::int32_t x, std::int32_t y) = 0;

    //! Register a system hotkey
    /*!
    Registers a system-wide hotkey.  The screen should arrange for an event
    to be delivered to itself when the hot key is pressed or released.  When
    that happens the screen should post a \c getHotKeyDownEvent() or
    \c getHotKeyUpEvent(), respectively.  The hot key is key \p key with
    exactly the modifiers \p mask.  Returns 0 on failure otherwise an id
    that can be used to unregister the hotkey.

    A hot key is a set of modifiers and a key, which may itself be a modifier.
    The hot key is pressed when the hot key's modifiers and only those
    modifiers are logically down (active) and the key is pressed.  The hot
    key is released when the key is released, regardless of the modifiers.

    The hot key event should be generated no matter what window or application
    has the focus.  No other window or application should receive the key
    press or release events (they can and should see the modifier key events).
    When the key is a modifier, it's acceptable to allow the user to press
    the modifiers in any order or to require the user to press the given key
    last.
    */
    virtual std::uint32_t registerHotKey(KeyID key, KeyModifierMask mask) = 0;

    //! Unregister a system hotkey
    /*!
    Unregisters a previously registered hot key.
    */
    virtual void unregisterHotKey(std::uint32_t id) = 0;

    //! Prepare to synthesize input on primary screen
    /*!
    Prepares the primary screen to receive synthesized input.  We do not
    want to receive this synthesized input as user input so this method
    ensures that we ignore it.  Calls to \c fakeInputBegin() may not be
    nested.
    */
    virtual void fakeInputBegin() = 0;

    //! Done synthesizing input on primary screen
    /*!
    Undoes whatever \c fakeInputBegin() did.
    */
    virtual void fakeInputEnd() = 0;

    //@}
    //! @name accessors
    //@{

    //! Get jump zone size
    /*!
    Return the jump zone size, the size of the regions on the edges of
    the screen that cause the cursor to jump to another screen.
    */
    virtual std::int32_t getJumpZoneSize() const = 0;

    //! Test if mouse is pressed
    /*!
    Return true if any mouse button is currently pressed.  Ideally,
    "current" means up to the last processed event but it can mean
    the current physical mouse button state.
    */
    virtual bool isAnyMouseButtonDown(std::uint32_t& buttonID) const = 0;

    //! Get cursor center position
    /*!
    Return the cursor center position which is where we park the
    cursor to compute cursor motion deltas and should be far from
    the edges of the screen, typically the center.
    */
    virtual void getCursorCenter(std::int32_t& x, std::int32_t& y) const = 0;

    //@}
};

} // namespace inputleap
