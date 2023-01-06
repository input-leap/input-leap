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

#include "inputleap/IKeyState.h"
#include "inputleap/KeyMap.h"

//! Core key state
/*!
This class provides key state services.  Subclasses must implement a few
platform specific methods.
*/
class KeyState : public IKeyState {
public:
    KeyState(IEventQueue* events);
    KeyState(IEventQueue* events, inputleap::KeyMap& keyMap);
    virtual ~KeyState();

    //! @name manipulators
    //@{

    //! Handle key event
    /*!
    Sets the state of \p button to down or up and updates the current
    modifier state to \p newState.  This method should be called by
    primary screens only in response to local events.  For auto-repeat
    set \p down to \c true.  Overrides must forward to the superclass.
    */
    virtual void onKey(KeyButton button, bool down,
                            KeyModifierMask newState);

    //! Post a key event
    /*!
    Posts a key event.  This may adjust the event or post additional
    events in some circumstances.  If this is overridden it must forward
    to the superclass.
    */
    virtual void sendKeyEvent(void* target, bool press, bool isAutoRepeat, KeyID key,
                              KeyModifierMask mask, std::int32_t count, KeyButton button);

    //@}
    //! @name accessors
    //@{

    //@}

    // IKeyState overrides
    void updateKeyMap() override;
    void updateKeyState() override;
    void setHalfDuplexMask(KeyModifierMask) override;
    void fakeKeyDown(KeyID id, KeyModifierMask mask, KeyButton button) override;
    bool fakeKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count,
                       KeyButton button) override;
    bool fakeKeyUp(KeyButton button) override;
    void fakeAllKeysUp() override;
    bool fakeMediaKey(KeyID id) override;

    bool isKeyDown(KeyButton) const override;
    KeyModifierMask getActiveModifiers() const override;

    std::int32_t getKeyState(KeyButton keyButton) { return m_keys[keyButton]; }

protected:
    typedef inputleap::KeyMap::Keystroke Keystroke;

    //! @name protected manipulators
    //@{

    //! Get the keyboard map
    /*!
    Fills \p keyMap with the current keyboard map.
    */
    virtual void getKeyMap(inputleap::KeyMap& keyMap) = 0;

    //! Fake a key event
    /*!
    Synthesize an event for \p keystroke.
    */
    virtual void fakeKey(const Keystroke& keystroke) = 0;

    //! Get the active modifiers
    /*!
    Returns the modifiers that are currently active according to our
    shadowed state.  The state may be modified.
    */
    virtual KeyModifierMask&
                        getActiveModifiersRValue();

    //@}
    //! @name protected accessors
    //@{

    //! Compute a group number
    /*!
    Returns the number of the group \p offset groups after group \p group.
    */
    std::int32_t getEffectiveGroup(std::int32_t group, std::int32_t offset) const;

    //! Check if key is ignored
    /*!
    Returns \c true if and only if the key should always be ignored.
    The default returns \c true only for the toggle keys.
    */
    virtual bool isIgnoredKey(KeyID key, KeyModifierMask mask) const;

    //! Get button for a KeyID
    /*!
    Return the button mapped to key \p id in group \p group if any,
    otherwise returns 0.
    */
    KeyButton getButton(KeyID id, std::int32_t group) const;

    //@}

private:
    typedef inputleap::KeyMap::Keystrokes Keystrokes;
    typedef inputleap::KeyMap::ModifierToKeys ModifierToKeys;
public:
    struct AddActiveModifierContext {
    public:
        AddActiveModifierContext(std::int32_t group, KeyModifierMask mask,
                                 ModifierToKeys& activeModifiers);

    public:
        std::int32_t m_activeGroup;
        KeyModifierMask m_mask;
        ModifierToKeys& m_activeModifiers;

    private:
        // not implemented
        AddActiveModifierContext(const AddActiveModifierContext&);
        AddActiveModifierContext& operator=(const AddActiveModifierContext&);
    };
private:

    class ButtonToKeyLess {
    public:
        bool operator()(const inputleap::KeyMap::ButtonToKeyMap::value_type& a,
                        const inputleap::KeyMap::ButtonToKeyMap::value_type b) const
        {
            return (a.first < b.first);
        }
    };

    // not implemented
    KeyState(const KeyState&);
    KeyState& operator=(const KeyState&);

    // called by all ctors.
    void init();

    // adds alias key sequences.  these are sequences that are equivalent
    // to other sequences.
    void addAliasEntries();

    // adds non-keypad key sequences for keypad KeyIDs
    void addKeypadEntries();

    // adds key sequences for combination KeyIDs (those built using
    // dead keys)
    void addCombinationEntries();

    // synthesize key events.  synthesize auto-repeat events count times.
    void fakeKeys(const Keystrokes&, std::uint32_t count);

    // update key state to match changes to modifiers
    void updateModifierKeyState(KeyButton button,
                            const ModifierToKeys& oldModifiers,
                            const ModifierToKeys& newModifiers);

    // active modifiers collection callback
    static void addActiveModifierCB(KeyID id, std::int32_t group,
                                    inputleap::KeyMap::KeyItem& keyItem, void* vcontext);

private:
    // must be declared before m_keyMap. used when this class owns the key map.
    inputleap::KeyMap* m_keyMapPtr;

    // the keyboard map
    inputleap::KeyMap& m_keyMap;

    // current modifier state
    KeyModifierMask m_mask;

    // the active modifiers and the buttons activating them
    ModifierToKeys m_activeModifiers;

    // current keyboard state (> 0 if pressed, 0 otherwise).  this is
    // initialized to the keyboard state according to the system then
    // it tracks synthesized events.
    std::int32_t m_keys[kNumButtons];

    // synthetic keyboard state (> 0 if pressed, 0 otherwise).  this
    // tracks the synthesized keyboard state.  if m_keys[n] > 0 but
    // m_syntheticKeys[n] == 0 then the key was pressed locally and
    // not synthesized yet.
    std::int32_t m_syntheticKeys[kNumButtons];

    // client data for each pressed key
    std::uint32_t m_keyClientData[kNumButtons];

    // server keyboard state.  an entry is 0 if not the key isn't pressed
    // otherwise it's the local KeyButton synthesized for the server key.
    KeyButton m_serverKeys[kNumButtons];

    IEventQueue* m_events;
};
