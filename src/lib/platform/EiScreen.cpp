/*  InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "platform/EiScreen.h"

#include "platform/EiEventQueueBuffer.h"
#include "platform/PortalRemoteDesktop.h"
#include "platform/PortalInputCapture.h"
#include "platform/EiKeyState.h"
#include "inputleap/Clipboard.h"
#include "inputleap/KeyMap.h"
#include "inputleap/XScreen.h"
#include "arch/XArch.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "base/IEventQueue.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <vector>

#include <libei.h>

namespace inputleap {

EiScreen::EiScreen(bool is_primary, IEventQueue* events, bool use_portal) :
    is_primary_(is_primary),
    events_(events),
    is_on_screen_(is_primary)
{
    if (is_primary)
        ei_ = ei_new_receiver(nullptr); // we receive from the display server
    else
        ei_ = ei_new_sender(nullptr); // we send to the display server
    ei_log_set_priority(ei_, EI_LOG_PRIORITY_DEBUG);
    ei_configure_name(ei_, "InputLeap client");

    key_state_ = new EiKeyState(this, events);
    // install event handlers
    events_->add_handler(EventType::SYSTEM, events_->getSystemTarget(),
                         [this](const auto& e){ handle_system_event(e); });

    if (use_portal) {
        events_->add_handler(EventType::EI_SCREEN_CONNECTED_TO_EIS, get_event_target(),
                             [this](const auto& e){ handle_connected_to_eis_event(e); });
        if (is_primary) {
#if HAVE_LIBPORTAL_INPUTCAPTURE
            portal_input_capture_ = new PortalInputCapture(this, events_);
#else
            throw std::invalid_argument("Missing libportal InputCapture portal support");
#endif
        } else {
            portal_remote_desktop_ = new PortalRemoteDesktop(this, events_);
        }
    } else {
        auto rc = ei_setup_backend_socket(ei_, NULL);
        if (rc != 0) {
            LOG((CLOG_DEBUG "ei init error: %s", strerror(-rc)));
            throw std::runtime_error("Failed to init ei context");
        }
    }

    // install the platform event queue
    events_->set_buffer(std::make_unique<EiEventQueueBuffer>(this, ei_, events_));
}

EiScreen::~EiScreen()
{
    events_->set_buffer(nullptr);
    events_->remove_handler(EventType::SYSTEM, events_->getSystemTarget());

    ei_device_unref(ei_pointer_);
    ei_device_unref(ei_keyboard_);
    ei_device_unref(ei_abs_);
    ei_seat_unref(ei_seat_);
    for (auto it = ei_devices_.begin(); it != ei_devices_.end(); it++) {
        ei_device_unref(*it);
    }
    ei_devices_.clear();
    ei_unref(ei_);

    delete portal_remote_desktop_;
#if HAVE_LIBPORTAL_INPUTCAPTURE
    delete portal_input_capture_;
#endif
}

const EventTarget* EiScreen::get_event_target() const
{
    return this;
}

bool EiScreen::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    return false;
}

void EiScreen::getShape(int32_t& x, int32_t& y, int32_t& w, int32_t& h) const
{
    x = x_;
    y = y_;
    w = w_;
    h = h_;
}

void EiScreen::getCursorPos(int32_t& x, int32_t& y) const
{
    // We cannot get the cursor position on EI, so we
    // always return the center of the screen
    x = x_ + w_/2;
    y = y_ + y_/2;
}

void EiScreen::reconfigure(uint32_t)
{
    // do nothing
}

void EiScreen::warpCursor(int32_t x, int32_t y)
{
    cursor_x_ = x;
    cursor_y_ = y;
}

std::uint32_t EiScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
    // FIXME
    return 0;
}

void EiScreen::unregisterHotKey(uint32_t id)
{
    // FIXME
}

void EiScreen::fakeInputBegin()
{
    // FIXME -- not implemented
}

void EiScreen::fakeInputEnd()
{
    // FIXME -- not implemented
}

std::int32_t EiScreen::getJumpZoneSize() const
{
    return 1;
}

bool EiScreen::isAnyMouseButtonDown(uint32_t& buttonID) const
{
    return false;
}

void EiScreen::getCursorCenter(int32_t& x, int32_t& y) const
{
    x = x_ + w_/2;
    y = y_ + h_/2;
}

void EiScreen::fakeMouseButton(ButtonID button, bool press)
{
    uint32_t code;

    if (!ei_pointer_)
        return;

    switch (button) {
    case kButtonLeft:   code = 0x110; break; // BTN_LEFT
    case kButtonMiddle: code = 0x112; break; // BTN_MIDDLE
    case kButtonRight:  code = 0x111; break; // BTN_RIGHT
    default:
        code = 0x110 + (button - 1);
        break;
    }

    ei_device_pointer_button(ei_pointer_, code, press);
    ei_device_frame(ei_pointer_, ei_now(ei_));
}

void EiScreen::fakeMouseMove(int32_t x, int32_t y)
{
    if (!ei_abs_)
        return;

    ei_device_pointer_motion_absolute(ei_abs_, x, y);
    ei_device_frame(ei_abs_, ei_now(ei_));
}

void EiScreen::fakeMouseRelativeMove(int32_t dx, int32_t dy) const
{
    if (!ei_pointer_)
        return;

    ei_device_pointer_motion(ei_pointer_, dx, dy);
    ei_device_frame(ei_pointer_, ei_now(ei_));
}

void EiScreen::fakeMouseWheel(int32_t xDelta, int32_t yDelta) const
{
    if (!ei_pointer_)
        return;

    // libEI and InputLeap seem to use opposite directions, so we have
    // to send EI the opposite of the value received if we want to remain
    // compatible with other platforms (including X11).
    ei_device_pointer_scroll_discrete(ei_pointer_, -xDelta, -yDelta);
    ei_device_frame(ei_pointer_, ei_now(ei_));
}

void EiScreen::fakeKey(uint32_t keycode, bool is_down) const
{
    key_state_->update_xkb_state(keycode, is_down);
    ei_device_keyboard_key(ei_keyboard_, keycode, is_down);
    ei_device_frame(ei_keyboard_, ei_now(ei_));
}

void EiScreen::enable()
{
    // Nothing really to be done here
}

void EiScreen::disable()
{
    // Nothing really to be done here, maybe cleanup in the future but ideally
    // that's handled elsewhere
}

void EiScreen::enter()
{
    is_on_screen_ = true;
    if (!is_primary_) {
        if (ei_pointer_) {
            ei_device_start_emulating(ei_pointer_);
        }
        if (ei_keyboard_) {
            ei_device_start_emulating(ei_keyboard_);
        }
        if (ei_abs_) {
            ei_device_start_emulating(ei_abs_);
        }
    }
#if HAVE_LIBPORTAL_INPUTCAPTURE
    else {
        LOG((CLOG_DEBUG "Releasing input capture"));
        portal_input_capture_->release(cursor_x_, cursor_y_);
    }
#endif
}

bool EiScreen::leave()
{
    if (!is_primary_) {
        if (ei_pointer_) {
            ei_device_stop_emulating(ei_pointer_);
        }
        if (ei_keyboard_) {
            ei_device_stop_emulating(ei_keyboard_);
        }
        if (ei_abs_) {
            ei_device_stop_emulating(ei_abs_);
        }
    }

    is_on_screen_ = false;
    return true;
}

bool EiScreen::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
    return false;
}

void EiScreen::checkClipboards()
{
    // do nothing, we're always up to date
}

void EiScreen::openScreensaver(bool notify)
{
    // FIXME
}

void EiScreen::closeScreensaver()
{
    // FIXME
}

void EiScreen::screensaver(bool activate)
{
    // FIXME
}

void
EiScreen::resetOptions()
{
    // Should reset options to neutral, see setOptions().
    // We don't have ei-specific options, nothing to do here
}

void EiScreen::setOptions(const OptionsList& options)
{
    // We don't have ei-specific options, nothing to do here
}

void EiScreen::setSequenceNumber(uint32_t seqNum)
{
    // FIXME: what is this used for?
}

bool EiScreen::isPrimary() const
{
    return is_primary_;
}

void EiScreen::update_shape()
{

    for (auto it = ei_devices_.begin(); it != ei_devices_.end(); it++) {
        auto idx = 0;
        struct ei_region *r;
        while ((r = ei_device_get_region(*it, idx++)) != NULL) {
            x_ = std::min(ei_region_get_x(r), x_);
            y_ = std::min(ei_region_get_y(r), y_);
            w_ = std::max(ei_region_get_x(r) + ei_region_get_width(r), w_);
            h_ = std::max(ei_region_get_y(r) + ei_region_get_height(r), h_);
        }
    }

    LOG((CLOG_NOTE "Logical output size: %dx%d@%d.%d", w_, h_, x_, y_));
}

void EiScreen::add_device(struct ei_device *device)
{
    LOG((CLOG_DEBUG "adding device %s", ei_device_get_name(device)));

    // Noteworthy: EI in principle supports multiple devices with multiple
    // capabilities, so there may be more than one logical pointer (or even
    // multiple seats). Supporting this is ... tricky so for now we go the easy
    // route: one device for each capability. Note this may be the same device
    // if the first device comes with multiple capabilities.

    if (!ei_pointer_ && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER))
        ei_pointer_ = ei_device_ref(device);

    if (!ei_keyboard_ && ei_device_has_capability(device, EI_DEVICE_CAP_KEYBOARD)) {
        ei_keyboard_ = ei_device_ref(device);

        struct ei_keymap *keymap = ei_device_keyboard_get_keymap(device);
        if (keymap && ei_keymap_get_type(keymap) == EI_KEYMAP_TYPE_XKB) {
            int fd = ei_keymap_get_fd(keymap);
            size_t len = ei_keymap_get_size(keymap);
            key_state_->init(fd, len);
        } else {
            // We rely on the EIS implementation to give us a keymap, otherwise we really have no
            // idea what a keycode means (other than it's linux/input.h code)
            // Where the EIS implementation does not tell us, we just default to
            // whatever libxkbcommon thinks is default. At least this way we can
            // influence with env vars what we get
            LOG((CLOG_WARN "keyboard device %s does not have a keymap, we are guessing", ei_device_get_name(device)));
            key_state_->init_default_keymap();
        }
        key_state_->updateKeyMap();
    }

    if (!ei_abs_ && ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE))
        ei_abs_ = ei_device_ref(device);

    ei_devices_.emplace_back(ei_device_ref(device));

    update_shape();
}

void EiScreen::remove_device(struct ei_device *device)
{
    LOG((CLOG_DEBUG "removing device %s", ei_device_get_name(device)));

    if (device == ei_pointer_)
        ei_pointer_ = ei_device_unref(ei_pointer_);
    if (device == ei_keyboard_)
        ei_keyboard_ = ei_device_unref(ei_keyboard_);
    if (device == ei_abs_)
        ei_abs_ = ei_device_unref(ei_abs_);

    for (auto it = ei_devices_.begin(); it != ei_devices_.end(); it++) {
        if (*it == device) {
            ei_devices_.erase(it);
            ei_device_unref(device);
            break;
        }
    }

    update_shape();
}

void EiScreen::send_event(EventType type, EventDataBase* data)
{
    events_->add_event(type, get_event_target(), data);
}

ButtonID EiScreen::map_button_from_evdev(ei_event* event) const
{
    uint32_t button = ei_event_pointer_get_button(event);

    switch (button)
    {
        case 0x110:
            return kButtonLeft;
        case 0x111:
            return kButtonRight;
        case 0x112:
            return kButtonMiddle;
        case 0x113:
            return kButtonExtra0;
        case 0x114:
            return kButtonExtra1;
        default:
            return kButtonNone;
    }

    return kButtonNone;
}

void EiScreen::on_key_event(ei_event* event)
{
    uint32_t keycode = ei_event_keyboard_get_key(event);
    uint32_t keyval = keycode + 8;
    bool pressed = ei_event_keyboard_get_key_is_press(event);
    KeyID keyid = key_state_->map_key_from_keyval(keyval);
    KeyButton keybutton = static_cast<KeyButton>(keyval);

    key_state_->update_xkb_state(keyval, pressed);
    KeyModifierMask mask = key_state_->pollActiveModifiers();

    LOG((CLOG_DEBUG1 "event: Key %s keycode=%d keyid=%d mask=0x%x", pressed ? "press" : "release", keycode, keyid, mask));

    if (keyid != kKeyNone) {
        key_state_->sendKeyEvent(get_event_target(), pressed, false, keyid,
                                 mask, 1, keybutton);
    }
}

void EiScreen::on_button_event(ei_event* event)
{
    LOG((CLOG_DEBUG "on_button_event"));
    assert(is_primary_);

    ButtonID button = map_button_from_evdev(event);
    bool pressed = ei_event_pointer_get_button_is_press(event);
    KeyModifierMask mask = key_state_->pollActiveModifiers();

    LOG((CLOG_DEBUG1 "event: Button %s button=%d mask=0x%x", pressed ? "press" : "release", button, mask));

    if (button == kButtonNone) {
        LOG((CLOG_DEBUG "onButtonEvent: button not recognized"));
        return;
    }

    send_event(pressed ? EventType::PRIMARY_SCREEN_BUTTON_DOWN
                       : EventType::PRIMARY_SCREEN_BUTTON_UP,
               create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
}

void EiScreen::on_pointer_scroll_event(ei_event* event)
{
    LOG((CLOG_DEBUG "on_pointer_scroll_event"));
    assert(is_primary_);

    double dx = ei_event_pointer_get_scroll_x(event);
    double dy = ei_event_pointer_get_scroll_y(event);

    LOG((CLOG_DEBUG1 "event: Scroll (%.1f,%.1f)", dx, dy));

    // libEI and InputLeap seem to use opposite directions, so we have
    // to send the opposite of the value reported by EI if we want to
    // remain compatible with other platforms (including X11).
    send_event(EventType::PRIMARY_SCREEN_WHEEL,
               create_event_data<WheelInfo>(WheelInfo{static_cast<std::int32_t>(-dx),
                                                      static_cast<std::int32_t>(-dy)}));
}

void EiScreen::on_pointer_scroll_discrete_event(ei_event* event)
{
    LOG((CLOG_DEBUG "on_pointer_scroll_discrete_event"));
    assert(is_primary_);

    double dx = ei_event_pointer_get_scroll_discrete_x(event);
    double dy = ei_event_pointer_get_scroll_discrete_y(event);

    LOG((CLOG_DEBUG1 "event: Scroll (%.1f,%.1f)", dx, dy));

    // libEI and InputLeap seem to use opposite directions, so we have
    // to send the opposite of the value reported by EI if we want to
    // remain compatible with other platforms (including X11).
    send_event(EventType::PRIMARY_SCREEN_WHEEL,
               create_event_data<WheelInfo>(WheelInfo{static_cast<std::int32_t>(-dx * 120),
                                                      static_cast<std::int32_t>(-dy * 120)}));
}

void EiScreen::on_motion_event(ei_event* event)
{
    LOG((CLOG_DEBUG "on_motion_event"));
    assert(is_primary_);

    double dx = ei_event_pointer_get_dx(event);
    double dy = ei_event_pointer_get_dy(event);

    cursor_x_ += dx;
    cursor_y_ += dy;

    if (is_on_screen_) {
        // motion on primary screen
        send_event(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY,
                   create_event_data<MotionInfo>(MotionInfo{cursor_x_, cursor_y_}));
    } else {
        // motion on secondary screen
        send_event(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY,
                   create_event_data<MotionInfo>(MotionInfo{static_cast<std::int32_t>(dx),
                                                            static_cast<std::int32_t>(dy)}));
    }
}

void EiScreen::on_abs_motion_event(ei_event* event)
{
    assert(is_primary_);
}

void EiScreen::handle_connected_to_eis_event(const Event& event)
{
    int fd = event.get_data_as<int>();
    LOG((CLOG_DEBUG "We have an EIS connection! fd is %d", fd));

    auto rc = ei_setup_backend_fd(ei_, fd);
    if (rc != 0) {
        LOG((CLOG_NOTE "Failed to set up ei: %s", strerror(-rc)));
    }
}

void EiScreen::handle_system_event(const Event& sysevent)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Only one ei_dispatch per system event, see the comment in
    // EiEventQueueBuffer::addEvent
    ei_dispatch(ei_);
    struct ei_event * event;

    while ((event = ei_get_event(ei_)) != nullptr) {
        auto type = ei_event_get_type(event);
        auto seat = ei_event_get_seat(event);
        auto device = ei_event_get_device(event);

        switch (type) {
            case EI_EVENT_CONNECT:
                LOG((CLOG_DEBUG "connected to EIS"));
                break;
            case EI_EVENT_SEAT_ADDED:
                if (!ei_seat_) {
                    ei_seat_ = ei_seat_ref(seat);
                    ei_seat_bind_capability(ei_seat_, EI_DEVICE_CAP_POINTER);
                    ei_seat_bind_capability(ei_seat_, EI_DEVICE_CAP_POINTER_ABSOLUTE);
                    ei_seat_bind_capability(ei_seat_, EI_DEVICE_CAP_KEYBOARD);
                    LOG((CLOG_DEBUG "using seat %s", ei_seat_get_name(ei_seat_)));
                    // we don't care about touch
                }
                break;
            case EI_EVENT_DEVICE_ADDED:
                if (seat == ei_seat_) {
                    add_device(device);
                } else {
                    LOG((CLOG_INFO "seat %s is ignored", ei_seat_get_name(ei_seat_)));
                }
                break;
            case EI_EVENT_DEVICE_REMOVED:
                remove_device(device);
                break;
            case EI_EVENT_SEAT_REMOVED:
                if (seat == ei_seat_) {
                    ei_seat_ = ei_seat_unref(ei_seat_);
                }
                break;
            case EI_EVENT_DISCONNECT:
                throw std::runtime_error("Oops, EIS didn't like us");
            case EI_EVENT_DEVICE_PAUSED:
                LOG((CLOG_DEBUG "device %s is paused", ei_device_get_name(device)));
            case EI_EVENT_DEVICE_RESUMED:
                LOG((CLOG_DEBUG "device %s is resumed", ei_device_get_name(device)));
                break;
            case EI_EVENT_PROPERTY:
                LOG((CLOG_DEBUG "property %s: %s", ei_event_property_get_name(event),
                     ei_event_property_get_value(event)));
                break;
            case EI_EVENT_KEYBOARD_MODIFIERS:
                // FIXME
                break;

            // events below are for a receiver context (barriers)
            case EI_EVENT_FRAME:
                break;
            case EI_EVENT_DEVICE_START_EMULATING:
                LOG((CLOG_DEBUG "device %s starts emulating", ei_device_get_name(device)));
                break;
            case EI_EVENT_DEVICE_STOP_EMULATING:
                LOG((CLOG_DEBUG "device %s stops emulating", ei_device_get_name(device)));
                break;
            case EI_EVENT_KEYBOARD_KEY:
                on_key_event(event);
                break;
            case EI_EVENT_POINTER_BUTTON:
                on_button_event(event);
                break;
            case EI_EVENT_POINTER_MOTION:
                on_motion_event(event);
                break;
            case EI_EVENT_POINTER_MOTION_ABSOLUTE:
                on_abs_motion_event(event);
                break;
            case EI_EVENT_TOUCH_UP:
                break;
            case EI_EVENT_TOUCH_MOTION:
                break;
            case EI_EVENT_TOUCH_DOWN:
                break;
            case EI_EVENT_POINTER_SCROLL:
                on_pointer_scroll_event(event);
                break;
            case EI_EVENT_POINTER_SCROLL_DISCRETE:
                on_pointer_scroll_discrete_event(event);
                break;
            case EI_EVENT_POINTER_SCROLL_STOP:
            case EI_EVENT_POINTER_SCROLL_CANCEL:
                break;
        }
        ei_event_unref(event);
    }
}

void EiScreen::updateButtons()
{
    // libei relies on the EIS implementation to keep our button count correct,
    // so there's not much we need to/can do here.
}

IKeyState* EiScreen::getKeyState() const
{
    return key_state_;
}

} // namespace inputleap
