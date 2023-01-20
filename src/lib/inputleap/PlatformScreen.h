/*
 * InputLeap -- mouse and keyboard sharing utility
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

#include "base/EventTarget.h"
#include "inputleap/IPlatformScreen.h"
#include "inputleap/DragInformation.h"
#include <stdexcept>

namespace inputleap {

//! Base screen implementation
/*!
This screen implementation is the superclass of all other screen
implementations.  It implements a handful of methods and requires
subclasses to implement the rest.
*/
class PlatformScreen : public IPlatformScreen, public EventTarget {
public:
    PlatformScreen();
    ~PlatformScreen() override;

    // IKeyState overrides
    void updateKeyMap() override;
    void updateKeyState() override;
    void setHalfDuplexMask(KeyModifierMask) override;
    void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button) override;
    bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                       KeyButton button) override;
    bool fakeKeyUp(KeyButton button) override;
    void fakeAllKeysUp() override;
    bool fakeCtrlAltDel() override;
    bool isKeyDown(KeyButton) const override;
    KeyModifierMask getActiveModifiers() const override;
    KeyModifierMask pollActiveModifiers() const override;
    std::int32_t pollActiveGroup() const override;
    void pollPressedKeys(KeyButtonSet& pressedKeys) const override;

    void setDraggingStarted(bool started) override { m_draggingStarted = started; }
    bool isDraggingStarted() override;
    bool isFakeDraggingStarted() override { return m_fakeDraggingStarted; }
    std::string& getDraggingFilename() override { return m_draggingFilename; }
    void clearDraggingFilename() override { }

    // IPlatformScreen overrides
    
    void fakeDraggingFiles(DragFileList fileList)  override
        { (void) fileList; throw std::runtime_error("fakeDraggingFiles not implemented"); }
    const std::string& getDropTarget() const override
        { throw std::runtime_error("getDropTarget not implemented"); }
    void setDropTarget(const std::string&) override
        { throw std::runtime_error("setDropTarget not implemented"); }

protected:
    //! Update mouse buttons
    /*!
    Subclasses must implement this method to update their internal mouse
    button mapping and, if desired, state tracking.
    */
    virtual void updateButtons() = 0;

    //! Get the key state
    /*!
    Subclasses must implement this method to return the platform specific
    key state object that each subclass must have.
    */
    virtual IKeyState* getKeyState() const = 0;

protected:
    std::string m_draggingFilename;
    bool m_draggingStarted;
    bool m_fakeDraggingStarted;
};

} // namespace inputleap
