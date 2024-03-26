/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2005 Chris Schoeneman
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

#include "server/InputFilter.h"
#include "server/Server.h"
#include "server/PrimaryClient.h"
#include "inputleap/KeyMap.h"
#include "base/EventQueue.h"
#include "base/Log.h"

#include <cstdlib>
#include <cstring>

namespace inputleap {

// -----------------------------------------------------------------------------
// Input Filter Condition Classes
// -----------------------------------------------------------------------------
InputFilter::Condition::Condition()
{
    // do nothing
}

InputFilter::Condition::~Condition()
{
    // do nothing
}

void
InputFilter::Condition::enablePrimary(PrimaryClient*)
{
    // do nothing
}

void
InputFilter::Condition::disablePrimary(PrimaryClient*)
{
    // do nothing
}

InputFilter::KeystrokeCondition::KeystrokeCondition(IEventQueue* events,
                                                    const IPlatformScreen::KeyInfo& info) :
    m_id(0),
    m_key(info.m_key),
    m_mask(info.m_mask),
    m_events(events)
{
}

InputFilter::KeystrokeCondition::KeystrokeCondition(
        IEventQueue* events, KeyID key, KeyModifierMask mask) :
    m_id(0),
    m_key(key),
    m_mask(mask),
    m_events(events)
{
    // do nothing
}

InputFilter::KeystrokeCondition::~KeystrokeCondition()
{
    // do nothing
}

KeyID
InputFilter::KeystrokeCondition::getKey() const
{
    return m_key;
}

KeyModifierMask
InputFilter::KeystrokeCondition::getMask() const
{
    return m_mask;
}

InputFilter::Condition*
InputFilter::KeystrokeCondition::clone() const
{
    return new KeystrokeCondition(m_events, m_key, m_mask);
}

std::string InputFilter::KeystrokeCondition::format() const
{
    return inputleap::string::sprintf("keystroke(%s)",
                            inputleap::KeyMap::formatKey(m_key, m_mask).c_str());
}

InputFilter::EFilterStatus
InputFilter::KeystrokeCondition::match(const Event& event)
{
    EFilterStatus status;

    // check for hotkey events
    EventType type = event.getType();
    if (type == EventType::PRIMARY_SCREEN_HOTKEY_DOWN) {
        status = kActivate;
    } else if (type == EventType::PRIMARY_SCREEN_HOTKEY_UP) {
        status = kDeactivate;
    }
    else {
        return kNoMatch;
    }

    // check if it's our hotkey
    const auto& kinfo = event.get_data_as<IPlatformScreen::HotKeyInfo>();
    if (kinfo.m_id != m_id) {
        return kNoMatch;
    }

    return status;
}

void
InputFilter::KeystrokeCondition::enablePrimary(PrimaryClient* primary)
{
    m_id = primary->registerHotKey(m_key, m_mask);
}

void
InputFilter::KeystrokeCondition::disablePrimary(PrimaryClient* primary)
{
    primary->unregisterHotKey(m_id);
    m_id = 0;
}

InputFilter::MouseButtonCondition::MouseButtonCondition(IEventQueue* events,
                                                        const IPrimaryScreen::ButtonInfo& info) :
    m_button(info.m_button),
    m_mask(info.m_mask),
    m_events(events)
{
}

InputFilter::MouseButtonCondition::MouseButtonCondition(
        IEventQueue* events, ButtonID button, KeyModifierMask mask) :
    m_button(button),
    m_mask(mask),
    m_events(events)
{
    // do nothing
}

InputFilter::MouseButtonCondition::~MouseButtonCondition()
{
    // do nothing
}

ButtonID
InputFilter::MouseButtonCondition::getButton() const
{
    return m_button;
}

KeyModifierMask
InputFilter::MouseButtonCondition::getMask() const
{
    return m_mask;
}

InputFilter::Condition*
InputFilter::MouseButtonCondition::clone() const
{
    return new MouseButtonCondition(m_events, m_button, m_mask);
}

std::string InputFilter::MouseButtonCondition::format() const
{
    std::string key = inputleap::KeyMap::formatKey(kKeyNone, m_mask);
    if (!key.empty()) {
        key += "+";
    }
    return inputleap::string::sprintf("mousebutton(%s%d)", key.c_str(), m_button);
}

InputFilter::EFilterStatus
InputFilter::MouseButtonCondition::match(const Event& event)
{
    static const KeyModifierMask s_ignoreMask =
        KeyModifierAltGr | KeyModifierCapsLock |
        KeyModifierNumLock | KeyModifierScrollLock;

    EFilterStatus status;

    // check for hotkey events
    EventType type = event.getType();
    if (type == EventType::PRIMARY_SCREEN_BUTTON_DOWN) {
        status = kActivate;
    } else if (type == EventType::PRIMARY_SCREEN_BUTTON_UP) {
        status = kDeactivate;
    }
    else {
        return kNoMatch;
    }

    // check if it's the right button and modifiers.  ignore modifiers
    // that cannot be combined with a mouse button.
    const auto& minfo = event.get_data_as<IPlatformScreen::ButtonInfo>();
    if (minfo.m_button != m_button || (minfo.m_mask & ~s_ignoreMask) != m_mask) {
        return kNoMatch;
    }

    return status;
}

InputFilter::ScreenConnectedCondition::ScreenConnectedCondition(IEventQueue* events,
                                                                const std::string& screen) :
    m_screen(screen),
    m_events(events)
{
    // do nothing
}

InputFilter::ScreenConnectedCondition::~ScreenConnectedCondition()
{
    // do nothing
}

InputFilter::Condition*
InputFilter::ScreenConnectedCondition::clone() const
{
    return new ScreenConnectedCondition(m_events, m_screen);
}

std::string InputFilter::ScreenConnectedCondition::format() const
{
    return inputleap::string::sprintf("connect(%s)", m_screen.c_str());
}

InputFilter::EFilterStatus
InputFilter::ScreenConnectedCondition::match(const Event& event)
{
    if (event.getType() == EventType::SERVER_CONNECTED) {
        const auto& info = event.get_data_as<Server::ScreenConnectedInfo>();
        if (m_screen == info.m_screen || m_screen.empty()) {
            return kActivate;
        }
    }

    return kNoMatch;
}

// -----------------------------------------------------------------------------
// Input Filter Action Classes
// -----------------------------------------------------------------------------
InputFilter::Action::Action()
{
    // do nothing
}

InputFilter::Action::~Action()
{
    // do nothing
}

InputFilter::LockCursorToScreenAction::LockCursorToScreenAction(
        IEventQueue* events, Mode mode) :
    m_mode(mode),
    m_events(events)
{
    // do nothing
}

InputFilter::LockCursorToScreenAction::Mode
InputFilter::LockCursorToScreenAction::getMode() const
{
    return m_mode;
}

InputFilter::Action*
InputFilter::LockCursorToScreenAction::clone() const
{
    return new LockCursorToScreenAction(*this);
}

std::string InputFilter::LockCursorToScreenAction::format() const
{
    static const char* s_mode[] = { "off", "on", "toggle" };

    return inputleap::string::sprintf("lockCursorToScreen(%s)", s_mode[m_mode]);
}

void
InputFilter::LockCursorToScreenAction::perform(const Event& event)
{
    static const Server::LockCursorToScreenInfo::State s_state[] = {
        Server::LockCursorToScreenInfo::kOff,
        Server::LockCursorToScreenInfo::kOn,
        Server::LockCursorToScreenInfo::kToggle
    };

    // send event
    Server::LockCursorToScreenInfo info(s_state[m_mode]);
    m_events->add_event(EventType::SERVER_LOCK_CURSOR_TO_SCREEN, event.getTarget(),
                        create_event_data<Server::LockCursorToScreenInfo>(info),
                        Event::kDeliverImmediately);
}

InputFilter::SwitchToScreenAction::SwitchToScreenAction(IEventQueue* events,
                                                        const std::string& screen) :
    m_screen(screen),
    m_events(events)
{
    // do nothing
}

std::string InputFilter::SwitchToScreenAction::getScreen() const
{
    return m_screen;
}

InputFilter::Action*
InputFilter::SwitchToScreenAction::clone() const
{
    return new SwitchToScreenAction(*this);
}

std::string InputFilter::SwitchToScreenAction::format() const
{
    return inputleap::string::sprintf("switchToScreen(%s)", m_screen.c_str());
}

void
InputFilter::SwitchToScreenAction::perform(const Event& event)
{
    // pick screen name.  if m_screen is empty then use the screen from
    // event if it has one.
    std::string screen = m_screen;
    if (screen.empty() && event.getType() == EventType::SERVER_CONNECTED) {
        const auto& info = event.get_data_as<Server::ScreenConnectedInfo>();
        screen = info.m_screen;
    }

    // send event
    Server::SwitchToScreenInfo info{screen};
    m_events->add_event(EventType::SERVER_SWITCH_TO_SCREEN, event.getTarget(),
                        create_event_data<Server::SwitchToScreenInfo>(info),
                        Event::kDeliverImmediately);
}

InputFilter::ToggleScreenAction::ToggleScreenAction(IEventQueue* events) :
    m_events(events)
{
    // do nothing
}

InputFilter::Action*
InputFilter::ToggleScreenAction::clone() const
{
    return new ToggleScreenAction(*this);
}

std::string InputFilter::ToggleScreenAction::format() const
{
    return inputleap::string::sprintf("toggleScreen");
}

void
InputFilter::ToggleScreenAction::perform(const Event& event)
{
    m_events->add_event(EventType::SERVER_TOGGLE_SCREEN, event.getTarget(), nullptr,
                        Event::kDeliverImmediately);
}

InputFilter::SwitchInDirectionAction::SwitchInDirectionAction(
        IEventQueue* events, EDirection direction) :
    m_direction(direction),
    m_events(events)
{
    // do nothing
}

EDirection
InputFilter::SwitchInDirectionAction::getDirection() const
{
    return m_direction;
}

InputFilter::Action*
InputFilter::SwitchInDirectionAction::clone() const
{
    return new SwitchInDirectionAction(*this);
}

std::string InputFilter::SwitchInDirectionAction::format() const
{
    static const char* s_names[] = {
        "",
        "left",
        "right",
        "up",
        "down"
    };

    return inputleap::string::sprintf("switchInDirection(%s)", s_names[m_direction]);
}

void
InputFilter::SwitchInDirectionAction::perform(const Event& event)
{
    Server::SwitchInDirectionInfo info{m_direction};
    m_events->add_event(EventType::SERVER_SWITCH_INDIRECTION, event.getTarget(),
                        create_event_data<Server::SwitchInDirectionInfo>(info),
                        Event::kDeliverImmediately);
}

InputFilter::KeyboardBroadcastAction::KeyboardBroadcastAction(
        IEventQueue* events, Mode mode) :
    m_mode(mode),
    m_events(events)
{
    // do nothing
}

InputFilter::KeyboardBroadcastAction::KeyboardBroadcastAction(IEventQueue* events, Mode mode,
                                                              const std::set<std::string>& screens) :
    m_mode(mode),
    m_screens(IKeyState::KeyInfo::join(screens)),
    m_events(events)
{
    // do nothing
}

InputFilter::KeyboardBroadcastAction::Mode
InputFilter::KeyboardBroadcastAction::getMode() const
{
    return m_mode;
}

std::set<std::string> InputFilter::KeyboardBroadcastAction::getScreens() const
{
    std::set<std::string> screens;
    IKeyState::KeyInfo::split(m_screens.c_str(), screens);
    return screens;
}

InputFilter::Action*
InputFilter::KeyboardBroadcastAction::clone() const
{
    return new KeyboardBroadcastAction(*this);
}

std::string InputFilter::KeyboardBroadcastAction::format() const
{
    static const char* s_mode[] = { "off", "on", "toggle" };
    static const char* s_name = "keyboardBroadcast";

    if (m_screens.empty() || m_screens[0] == '*') {
        return inputleap::string::sprintf("%s(%s)", s_name, s_mode[m_mode]);
    }
    else {
        return inputleap::string::sprintf("%s(%s,%.*s)", s_name, s_mode[m_mode],
                            m_screens.size() - 2,
                            m_screens.c_str() + 1);
    }
}

void
InputFilter::KeyboardBroadcastAction::perform(const Event& event)
{
    static const Server::KeyboardBroadcastInfo::State s_state[] = {
        Server::KeyboardBroadcastInfo::kOff,
        Server::KeyboardBroadcastInfo::kOn,
        Server::KeyboardBroadcastInfo::kToggle
    };

    // send event
    Server::KeyboardBroadcastInfo info{s_state[m_mode], m_screens};
    m_events->add_event(EventType::SERVER_KEYBOARD_BROADCAST, event.getTarget(),
                        create_event_data<Server::KeyboardBroadcastInfo>(info),
                        Event::kDeliverImmediately);
}

InputFilter::KeystrokeAction::KeystrokeAction(IEventQueue* events, const IKeyState::KeyInfo& info, bool press) :
    info_(info),
    m_press(press),
    m_events(events)
{
    // do nothing
}

InputFilter::KeystrokeAction::~KeystrokeAction() = default;

bool
InputFilter::KeystrokeAction::isOnPress() const
{
    return m_press;
}

InputFilter::Action*
InputFilter::KeystrokeAction::clone() const
{
    return new KeystrokeAction(m_events, info_, m_press);
}

std::string InputFilter::KeystrokeAction::format() const
{
    const char* type = formatName();

    if (info_.screens_.empty()) {
        return inputleap::string::sprintf("%s(%s)", type,
                            inputleap::KeyMap::formatKey(info_.m_key, info_.m_mask).c_str());
    }
    else if (info_.screens_ == "*") {
        return inputleap::string::sprintf("%s(%s,*)", type,
                            inputleap::KeyMap::formatKey(info_.m_key, info_.m_mask).c_str());
    }
    else {
        return inputleap::string::sprintf("%s(%s,%s)", type,
                            inputleap::KeyMap::formatKey(info_.m_key, info_.m_mask).c_str(),
                            info_.screens_.c_str());
    }
}

void
InputFilter::KeystrokeAction::perform(const Event& event)
{
    EventType type = m_press ? EventType::KEY_STATE_KEY_DOWN : EventType::KEY_STATE_KEY_UP;

    m_events->add_event(EventType::PRIMARY_SCREEN_FAKE_INPUT_BEGIN, event.getTarget(), nullptr,
                        Event::kDeliverImmediately);
    m_events->add_event(type, event.getTarget(),
                        create_event_data<IPlatformScreen::KeyInfo>(info_),
                        Event::kDeliverImmediately);
    m_events->add_event(EventType::PRIMARY_SCREEN_FAKE_INPUT_END, event.getTarget(), nullptr,
                        Event::kDeliverImmediately);
}

const char*
InputFilter::KeystrokeAction::formatName() const
{
    return (m_press ? "keyDown" : "keyUp");
}

InputFilter::MouseButtonAction::MouseButtonAction(IEventQueue* events,
                                                  const IPlatformScreen::ButtonInfo& info,
                                                  bool press) :
    button_info_(info),
    m_press(press),
    m_events(events)
{
    // do nothing
}

InputFilter::MouseButtonAction::~MouseButtonAction() = default;

bool
InputFilter::MouseButtonAction::isOnPress() const
{
    return m_press;
}

InputFilter::Action*
InputFilter::MouseButtonAction::clone() const
{
    return new MouseButtonAction(m_events, button_info_, m_press);
}

std::string InputFilter::MouseButtonAction::format() const
{
    const char* type = formatName();

    std::string key = inputleap::KeyMap::formatKey(kKeyNone, button_info_.m_mask);
    return inputleap::string::sprintf("%s(%s%s%d)", type,
                            key.c_str(), key.empty() ? "" : "+",
                            button_info_.m_button);
}

void
InputFilter::MouseButtonAction::perform(const Event& event)

{
    // send modifiers
    if (button_info_.m_mask != 0) {
        KeyID key = m_press ? kKeySetModifiers : kKeyClearModifiers;
        m_events->add_event(EventType::KEY_STATE_KEY_DOWN, event.getTarget(),
                            create_event_data<IPlatformScreen::KeyInfo>(
                                IPlatformScreen::KeyInfo{key, button_info_.m_mask, 0, 1}),
                            Event::kDeliverImmediately);
    }

    // send button
    EventType type = m_press ? EventType::PRIMARY_SCREEN_BUTTON_DOWN :
                               EventType::PRIMARY_SCREEN_BUTTON_UP;
    m_events->add_event(type, event.getTarget(),
                        create_event_data<IPlatformScreen::ButtonInfo>(button_info_),
                        Event::kDeliverImmediately);
}

const char*
InputFilter::MouseButtonAction::formatName() const
{
    return (m_press ? "mouseDown" : "mouseUp");
}

//
// InputFilter::Rule
//

InputFilter::Rule::Rule() :
    m_condition(nullptr)
{
    // do nothing
}

InputFilter::Rule::Rule(Condition* adoptedCondition) :
    m_condition(adoptedCondition)
{
    // do nothing
}

InputFilter::Rule::Rule(const Rule& rule) :
    m_condition(nullptr)
{
    copy(rule);
}

InputFilter::Rule::~Rule()
{
    clear();
}

InputFilter::Rule&
InputFilter::Rule::operator=(const Rule& rule)
{
    if (&rule != this) {
        copy(rule);
    }
    return *this;
}

void
InputFilter::Rule::clear()
{
    delete m_condition;
    for (ActionList::iterator i = m_activateActions.begin();
                                i != m_activateActions.end(); ++i) {
        delete *i;
    }
    for (ActionList::iterator i = m_deactivateActions.begin();
                                i != m_deactivateActions.end(); ++i) {
        delete *i;
    }

    m_condition = nullptr;
    m_activateActions.clear();
    m_deactivateActions.clear();
}

void
InputFilter::Rule::copy(const Rule& rule)
{
    clear();
    if (rule.m_condition != nullptr) {
        m_condition = rule.m_condition->clone();
    }
    for (ActionList::const_iterator i = rule.m_activateActions.begin();
                                i != rule.m_activateActions.end(); ++i) {
        m_activateActions.push_back((*i)->clone());
    }
    for (ActionList::const_iterator i = rule.m_deactivateActions.begin();
                                i != rule.m_deactivateActions.end(); ++i) {
        m_deactivateActions.push_back((*i)->clone());
    }
}

void
InputFilter::Rule::setCondition(Condition* adopted)
{
    delete m_condition;
    m_condition = adopted;
}

void
InputFilter::Rule::adoptAction(Action* action, bool onActivation)
{
    if (action != nullptr) {
        if (onActivation) {
            m_activateActions.push_back(action);
        }
        else {
            m_deactivateActions.push_back(action);
        }
    }
}

void InputFilter::Rule::removeAction(bool onActivation, std::uint32_t index)
{
    if (onActivation) {
        delete m_activateActions[index];
        m_activateActions.erase(m_activateActions.begin() + index);
    }
    else {
        delete m_deactivateActions[index];
        m_deactivateActions.erase(m_deactivateActions.begin() + index);
    }
}

void InputFilter::Rule::replaceAction(Action* adopted, bool onActivation, std::uint32_t index)
{
    if (adopted == nullptr) {
        removeAction(onActivation, index);
    }
    else if (onActivation) {
        delete m_activateActions[index];
        m_activateActions[index] = adopted;
    }
    else {
        delete m_deactivateActions[index];
        m_deactivateActions[index] = adopted;
    }
}

void
InputFilter::Rule::enable(PrimaryClient* primaryClient)
{
    if (m_condition != nullptr) {
        m_condition->enablePrimary(primaryClient);
    }
}

void
InputFilter::Rule::disable(PrimaryClient* primaryClient)
{
    if (m_condition != nullptr) {
        m_condition->disablePrimary(primaryClient);
    }
}

bool
InputFilter::Rule::handleEvent(const Event& event)
{
    // nullptr condition never matches
    if (m_condition == nullptr) {
        return false;
    }

    // match
    const ActionList* actions;
    switch (m_condition->match(event)) {
    default:
        // not handled
        return false;

    case kActivate:
        actions = &m_activateActions;
        LOG_DEBUG1("activate actions");
        break;

    case kDeactivate:
        actions = &m_deactivateActions;
        LOG_DEBUG1("deactivate actions");
        break;
    }

    // perform actions
    for (ActionList::const_iterator i = actions->begin();
                                i != actions->end(); ++i) {
        LOG_DEBUG1("hotkey: %s", (*i)->format().c_str());
        (*i)->perform(event);
    }

    return true;
}

std::string InputFilter::Rule::format() const
{
    std::string s;
    if (m_condition != nullptr) {
        // condition
        s += m_condition->format();
        s += " = ";

        // activate actions
        ActionList::const_iterator i = m_activateActions.begin();
        if (i != m_activateActions.end()) {
            s += (*i)->format();
            while (++i != m_activateActions.end()) {
                s += ", ";
                s += (*i)->format();
            }
        }

        // deactivate actions
        if (!m_deactivateActions.empty()) {
            s += "; ";
            i = m_deactivateActions.begin();
            if (i != m_deactivateActions.end()) {
                s += (*i)->format();
                while (++i != m_deactivateActions.end()) {
                    s += ", ";
                    s += (*i)->format();
                }
            }
        }
    }
    return s;
}

const InputFilter::Condition*
InputFilter::Rule::getCondition() const
{
    return m_condition;
}

std::uint32_t InputFilter::Rule::getNumActions(bool onActivation) const
{
    if (onActivation) {
        return static_cast<std::uint32_t>(m_activateActions.size());
    }
    else {
        return static_cast<std::uint32_t>(m_deactivateActions.size());
    }
}

const InputFilter::Action& InputFilter::Rule::getAction(bool onActivation,
                                                        std::uint32_t index) const
{
    if (onActivation) {
        return *m_activateActions[index];
    }
    else {
        return *m_deactivateActions[index];
    }
}


// -----------------------------------------------------------------------------
// Input Filter Class
// -----------------------------------------------------------------------------
InputFilter::InputFilter(IEventQueue* events) :
    m_primaryClient(nullptr),
    m_events(events)
{
    // do nothing
}

InputFilter::InputFilter(const InputFilter& x) :
    EventTarget(),
    m_ruleList(x.m_ruleList),
    m_primaryClient(nullptr),
    m_events(x.m_events)
{
    setPrimaryClient(x.m_primaryClient);
}

InputFilter::~InputFilter()
{
    setPrimaryClient(nullptr);
}

InputFilter&
InputFilter::operator=(const InputFilter& x)
{
    if (&x != this) {
        PrimaryClient* oldClient = m_primaryClient;
        setPrimaryClient(nullptr);

        m_ruleList = x.m_ruleList;

        setPrimaryClient(oldClient);
    }
    return *this;
}

void
InputFilter::addFilterRule(const Rule& rule)
{
    m_ruleList.push_back(rule);
    if (m_primaryClient != nullptr) {
        m_ruleList.back().enable(m_primaryClient);
    }
}

void InputFilter::removeFilterRule(std::uint32_t index)
{
    if (m_primaryClient != nullptr) {
        m_ruleList[index].disable(m_primaryClient);
    }
    m_ruleList.erase(m_ruleList.begin() + index);
}

InputFilter::Rule& InputFilter::getRule(std::uint32_t index)
{
    return m_ruleList[index];
}

void
InputFilter::setPrimaryClient(PrimaryClient* client)
{
    if (m_primaryClient == client) {
        return;
    }

    if (m_primaryClient != nullptr) {
        for (RuleList::iterator rule  = m_ruleList.begin();
                                 rule != m_ruleList.end(); ++rule) {
            rule->disable(m_primaryClient);
        }

        m_events->remove_handler(EventType::KEY_STATE_KEY_DOWN, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::KEY_STATE_KEY_UP, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::KEY_STATE_KEY_REPEAT, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::PRIMARY_SCREEN_BUTTON_DOWN, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::PRIMARY_SCREEN_BUTTON_UP, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::PRIMARY_SCREEN_HOTKEY_DOWN, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::PRIMARY_SCREEN_HOTKEY_UP, m_primaryClient->get_event_target());
        m_events->remove_handler(EventType::SERVER_CONNECTED, m_primaryClient->get_event_target());
    }

    m_primaryClient = client;

    if (m_primaryClient != nullptr) {
        auto event_target = m_primaryClient->get_event_target();
        m_events->add_handler(EventType::KEY_STATE_KEY_DOWN, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::KEY_STATE_KEY_UP, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::KEY_STATE_KEY_REPEAT, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::PRIMARY_SCREEN_BUTTON_DOWN, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::PRIMARY_SCREEN_BUTTON_UP, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::PRIMARY_SCREEN_HOTKEY_DOWN, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::PRIMARY_SCREEN_HOTKEY_UP, event_target,
                              [this](const auto& e){ handle_event(e); });
        m_events->add_handler(EventType::SERVER_CONNECTED, event_target,
                              [this](const auto& e){ handle_event(e); });

        for (RuleList::iterator rule  = m_ruleList.begin();
                                 rule != m_ruleList.end(); ++rule) {
            rule->enable(m_primaryClient);
        }
    }
}

std::string InputFilter::format(const std::string& linePrefix) const
{
    std::string s;
    for (RuleList::const_iterator i = m_ruleList.begin();
                                i != m_ruleList.end(); ++i) {
        s += linePrefix;
        s += i->format();
        s += "\n";
    }
    return s;
}

std::uint32_t InputFilter::getNumRules() const
{
    return static_cast<std::uint32_t>(m_ruleList.size());
}

bool
InputFilter::operator==(const InputFilter& x) const
{
    // if there are different numbers of rules then we can't be equal
    if (m_ruleList.size() != x.m_ruleList.size()) {
        return false;
    }

    // compare rule lists.  the easiest way to do that is to format each
    // rule into a string, sort the strings, then compare the results.
    std::vector<std::string> aList, bList;
    for (RuleList::const_iterator i = m_ruleList.begin();
                                i != m_ruleList.end(); ++i) {
        aList.push_back(i->format());
    }
    for (RuleList::const_iterator i = x.m_ruleList.begin();
                                i != x.m_ruleList.end(); ++i) {
        bList.push_back(i->format());
    }
    std::partial_sort(aList.begin(), aList.end(), aList.end());
    std::partial_sort(bList.begin(), bList.end(), bList.end());
    return (aList == bList);
}

bool
InputFilter::operator!=(const InputFilter& x) const
{
    return !operator==(x);
}

void InputFilter::handle_event(const Event& event)
{
    // copy event and adjust target
    Event myEvent(event.getType(), this, nullptr, event.getFlags() | Event::kDeliverImmediately);
    myEvent.clone_data_from(event);

    // let each rule try to match the event until one does
    for (RuleList::iterator rule  = m_ruleList.begin();
                             rule != m_ruleList.end(); ++rule) {
        if (rule->handleEvent(myEvent)) {
            // handled
            return;
        }
    }

    // not handled so pass through
    m_events->add_event(std::move(myEvent));
}

} // namespace inputleap
