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

#include "inputleap/key_types.h"
#include "inputleap/mouse_types.h"
#include "inputleap/protocol_types.h"
#include "inputleap/IPlatformScreen.h"
#include "common/stdmap.h"
#include "common/stdset.h"

class PrimaryClient;
class Event;
class IEventQueue;

class InputFilter {
public:
    // -------------------------------------------------------------------------
    // Input Filter Condition Classes
    // -------------------------------------------------------------------------
    enum EFilterStatus {
        kNoMatch,
        kActivate,
        kDeactivate
    };

    class Condition {
    public:
        Condition();
        virtual ~Condition();

        virtual Condition* clone() const = 0;
        virtual std::string format() const = 0;

        virtual EFilterStatus match(const Event&) = 0;

        virtual void enablePrimary(PrimaryClient*);
        virtual void disablePrimary(PrimaryClient*);
    };

    // KeystrokeCondition
    class KeystrokeCondition : public Condition {
    public:
        KeystrokeCondition(IEventQueue* events, IPlatformScreen::KeyInfo*);
        KeystrokeCondition(IEventQueue* events, KeyID key, KeyModifierMask mask);
        ~KeystrokeCondition() override;

        KeyID getKey() const;
        KeyModifierMask getMask() const;

        // Condition overrides
        Condition* clone() const override;
        std::string format() const override;
        EFilterStatus match(const Event&) override;
        void enablePrimary(PrimaryClient*) override;
        void disablePrimary(PrimaryClient*) override;

    private:
        std::uint32_t m_id;
        KeyID m_key;
        KeyModifierMask m_mask;
        IEventQueue* m_events;
    };

    // MouseButtonCondition
    class MouseButtonCondition : public Condition {
    public:
        MouseButtonCondition(IEventQueue* events, IPlatformScreen::ButtonInfo*);
        MouseButtonCondition(IEventQueue* events, ButtonID, KeyModifierMask mask);
        ~MouseButtonCondition() override;

        ButtonID getButton() const;
        KeyModifierMask getMask() const;

        // Condition overrides
        Condition* clone() const override;
        std::string format() const override;
        EFilterStatus match(const Event&) override;

    private:
        ButtonID m_button;
        KeyModifierMask m_mask;
        IEventQueue* m_events;
    };

    // ScreenConnectedCondition
    class ScreenConnectedCondition : public Condition {
    public:
        ScreenConnectedCondition(IEventQueue* events, const std::string& screen);
        ~ScreenConnectedCondition() override;

        // Condition overrides
        Condition* clone() const override;
        std::string format() const override;
        EFilterStatus match(const Event&) override;

    private:
        std::string m_screen;
        IEventQueue* m_events;
    };

    // -------------------------------------------------------------------------
    // Input Filter Action Classes
    // -------------------------------------------------------------------------

    class Action {
    public:
        Action();
        virtual    ~Action();

        virtual Action* clone() const = 0;
        virtual std::string format() const = 0;

        virtual void perform(const Event&) = 0;
    };

    // LockCursorToScreenAction
    class LockCursorToScreenAction : public Action {
    public:
        enum Mode { kOff, kOn, kToggle };

        LockCursorToScreenAction(IEventQueue* events, Mode = kToggle);

        Mode getMode() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    private:
        Mode m_mode;
        IEventQueue* m_events;
    };

    // SwitchToScreenAction
    class SwitchToScreenAction : public Action {
    public:
        SwitchToScreenAction(IEventQueue* events, const std::string& screen);

        std::string getScreen() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    private:
        std::string m_screen;
        IEventQueue* m_events;
    };

    // ToggleScreenAction
    class ToggleScreenAction : public Action {
    public:
        ToggleScreenAction(IEventQueue* events);

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    private:
        IEventQueue* m_events;
    };

    // SwitchInDirectionAction
    class SwitchInDirectionAction : public Action {
    public:
        SwitchInDirectionAction(IEventQueue* events, EDirection);

        EDirection getDirection() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    private:
        EDirection m_direction;
        IEventQueue* m_events;
    };

    // KeyboardBroadcastAction
    class KeyboardBroadcastAction : public Action {
    public:
        enum Mode { kOff, kOn, kToggle };

        KeyboardBroadcastAction(IEventQueue* events, Mode = kToggle);
        KeyboardBroadcastAction(IEventQueue* events, Mode, const std::set<std::string>& screens);

        Mode getMode() const;
        std::set<std::string> getScreens() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    private:
        Mode m_mode;
        std::string m_screens;
        IEventQueue* m_events;
    };

    // KeystrokeAction
    class KeystrokeAction : public Action {
    public:
        KeystrokeAction(IEventQueue* events, IPlatformScreen::KeyInfo* adoptedInfo, bool press);
        ~KeystrokeAction();

        void adoptInfo(IPlatformScreen::KeyInfo*);
        const IPlatformScreen::KeyInfo* getInfo() const;
        bool isOnPress() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    protected:
        virtual const char* formatName() const;

    private:
        IPlatformScreen::KeyInfo* m_keyInfo;
        bool m_press;
        IEventQueue* m_events;
    };

    // MouseButtonAction -- modifier combinations not implemented yet
    class MouseButtonAction : public Action {
    public:
        MouseButtonAction(IEventQueue* events,
                                    IPlatformScreen::ButtonInfo* adoptedInfo,
                                    bool press);
        ~MouseButtonAction();

        const IPlatformScreen::ButtonInfo* getInfo() const;
        bool isOnPress() const;

        // Action overrides
        Action* clone() const override;
        std::string format() const override;
        void perform(const Event&) override;

    protected:
        virtual const char* formatName() const;

    private:
        IPlatformScreen::ButtonInfo* m_buttonInfo;
        bool m_press;
        IEventQueue* m_events;
    };

    class Rule {
    public:
        Rule();
        Rule(Condition* adopted);
        Rule(const Rule&);
        ~Rule();

        Rule& operator=(const Rule&);

        // replace the condition
        void setCondition(Condition* adopted);

        // add an action to the rule
        void adoptAction(Action*, bool onActivation);

        // remove an action from the rule
        void removeAction(bool onActivation, std::uint32_t index);

        // replace an action in the rule
        void replaceAction(Action* adopted, bool onActivation, std::uint32_t index);

        // enable/disable
        void enable(PrimaryClient*);
        void disable(PrimaryClient*);

        // event handling
        bool handleEvent(const Event&);

        // convert rule to a string
        std::string format() const;

        // get the rule's condition
        const Condition* getCondition() const;

        // get number of actions
        std::uint32_t getNumActions(bool onActivation) const;

        // get action by index
        const Action& getAction(bool onActivation, std::uint32_t index) const;

    private:
        void clear();
        void copy(const Rule&);

    private:
        typedef std::vector<Action*> ActionList;

        Condition* m_condition;
        ActionList m_activateActions;
        ActionList m_deactivateActions;
    };

    // -------------------------------------------------------------------------
    // Input Filter Class
    // -------------------------------------------------------------------------
    typedef std::vector<Rule> RuleList;

    InputFilter(IEventQueue* events);
    InputFilter(const InputFilter&);
    virtual ~InputFilter();

#ifdef INPUTLEAP_TEST_ENV
    InputFilter() : m_primaryClient(nullptr) { }
#endif

    InputFilter&        operator=(const InputFilter&);

    // add rule, adopting the condition and the actions
    void addFilterRule(const Rule& rule);

    // remove a rule
    void removeFilterRule(std::uint32_t index);

    // get rule by index
    Rule& getRule(std::uint32_t index);

    // enable event filtering using the given primary client.  disable
    // if client is nullptr.
    virtual void setPrimaryClient(PrimaryClient* client);

    // convert rules to a string
    std::string format(const std::string& linePrefix) const;

    // get number of rules
    std::uint32_t getNumRules() const;

    //! Compare filters
    bool                operator==(const InputFilter&) const;
    //! Compare filters
    bool                operator!=(const InputFilter&) const;

private:
    // event handling
    void handleEvent(const Event&, void*);

private:
    RuleList m_ruleList;
    PrimaryClient* m_primaryClient;
    IEventQueue* m_events;
};
