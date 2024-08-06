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

#include <cmath>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <unistd.h>
#include <vector>

struct ScrollRemainder {
    double x, y; // scroll remainder in pixels
};

namespace inputleap {

EiScreen::EiScreen(bool is_primary, IEventQueue* events, bool use_portal) :
    is_primary_(is_primary),
    events_(events),
    w_(1),
    h_(1),
    is_on_screen_(is_primary)
{
    init_ei();
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
            events_->add_handler(EventType::EI_SESSION_CLOSED, get_event_target(),
                                 [this](const auto& e){ handle_portal_session_closed(e); });
            portal_remote_desktop_ = new PortalRemoteDesktop(this, events_);
        }
    } else {
        // Note: socket backend does not support reconnections
        auto rc = ei_setup_backend_socket(ei_, nullptr);
        if (rc != 0) {
            LOG_DEBUG("ei init error: %s", strerror(-rc));
            throw std::runtime_error("Failed to init ei context");
        }
    }
}

EiScreen::~EiScreen()
{
    events_->set_buffer(nullptr);
    events_->remove_handler(EventType::SYSTEM, events_->getSystemTarget());

    cleanup_ei();

    delete key_state_;

    delete portal_remote_desktop_;
#if HAVE_LIBPORTAL_INPUTCAPTURE
    delete portal_input_capture_;
#endif
}

void EiScreen::handle_ei_log_event(ei* ei,
                                   ei_log_priority priority,
                                   const char* message,
                                   ei_log_context* context)
{
    switch (priority) {
        case EI_LOG_PRIORITY_DEBUG:
            LOG_DEBUG3("ei: %s", message);
            break;
        case EI_LOG_PRIORITY_INFO:
            LOG_INFO("ei: %s", message);
            break;
        case EI_LOG_PRIORITY_WARNING:
            LOG_WARN("ei: %s", message);
            break;
        case EI_LOG_PRIORITY_ERROR:
            LOG_ERR("ei: %s", message);
            break;
        default:
            LOG_PRINT("ei: %s", message);
            break;
    }
}

void EiScreen::init_ei()
{
    if (is_primary_) {
        ei_ = ei_new_receiver(nullptr); // we receive from the display server
    } else {
        ei_ = ei_new_sender(nullptr); // we send to the display server
    }
    ei_set_user_data(ei_, this);
    ei_log_set_priority(ei_, EI_LOG_PRIORITY_DEBUG);
    ei_log_set_handler(ei_, cb_handle_ei_log_event);
    ei_configure_name(ei_, "InputLeap client");

    // install the platform event queue
    events_->set_buffer(nullptr);
    events_->set_buffer(std::make_unique<EiEventQueueBuffer>(this, ei_, events_));
}

void EiScreen::cleanup_ei()
{
    if (ei_pointer_) {
            free(ei_device_get_user_data(ei_pointer_));
            ei_device_set_user_data(ei_pointer_, nullptr);
            ei_pointer_ = ei_device_unref(ei_pointer_);
    }
    if (ei_keyboard_) {
            free(ei_device_get_user_data(ei_keyboard_));
            ei_device_set_user_data(ei_keyboard_, nullptr);
            ei_keyboard_ = ei_device_unref(ei_keyboard_);
    }
    if (ei_abs_) {
            free(ei_device_get_user_data(ei_abs_));
            ei_device_set_user_data(ei_abs_, nullptr);
            ei_abs_ = ei_device_unref(ei_abs_);
    }
    ei_seat_unref(ei_seat_);
    for (auto it = ei_devices_.begin(); it != ei_devices_.end(); it++) {
            free(ei_device_get_user_data(*it));
            ei_device_set_user_data(*it, nullptr);
            ei_device_unref(*it);
    }
    ei_devices_.clear();
    ei_ = ei_unref(ei_);
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
    x = cursor_x_;
    y = cursor_y_;
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
    static std::uint32_t next_id;
    std::uint32_t id = std::min(++next_id, 1u);

    // Bug: id rollover means duplicate hotkey ids. Oh well.

    auto set = hotkeys_.find(key);
    if (set == hotkeys_.end()) {
        hotkeys_.emplace(key, HotKeySet { key });
        set = hotkeys_.find(key);
    }
    set->second.add_item(HotKeyItem(mask, id));

    return id;
}

void EiScreen::unregisterHotKey(uint32_t id)
{
    for (auto& set : hotkeys_) {
        if (set.second.remove_by_id(id)) {
            break;
        }
    }
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

    ei_device_button_button(ei_pointer_, code, press);
    ei_device_frame(ei_pointer_, ei_now(ei_));
}

void EiScreen::fakeMouseMove(int32_t x, int32_t y)
{
    // We get one motion event before enter() with the target position
    if (!is_on_screen_) {
        cursor_x_ = x;
        cursor_y_ = y;
        return;
    }

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
    ei_device_scroll_discrete(ei_pointer_, -xDelta, -yDelta);
    ei_device_frame(ei_pointer_, ei_now(ei_));
}

void EiScreen::fakeKey(uint32_t keycode, bool is_down) const
{
    if (!ei_keyboard_)
        return;

    auto xkb_keycode = keycode + 8;
    key_state_->update_xkb_state(xkb_keycode, is_down);
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
        ++sequence_number_;
        if (ei_pointer_) {
            ei_device_start_emulating(ei_pointer_, sequence_number_);
        }
        if (ei_keyboard_) {
            ei_device_start_emulating(ei_keyboard_, sequence_number_);
        }
        if (ei_abs_) {
            ei_device_start_emulating(ei_abs_, sequence_number_);
            fakeMouseMove(cursor_x_, cursor_y_);
        }
    }
#if HAVE_LIBPORTAL_INPUTCAPTURE
    else {
        LOG_DEBUG("Releasing input capture at (cursor_x_,cursor_y_) = (%i,%i)",
             cursor_x_, cursor_y_);
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
        while ((r = ei_device_get_region(*it, idx++)) != nullptr) {
            x_ = std::min(ei_region_get_x(r), x_);
            y_ = std::min(ei_region_get_y(r), y_);
            w_ = std::max(ei_region_get_x(r) + ei_region_get_width(r), w_);
            h_ = std::max(ei_region_get_y(r) + ei_region_get_height(r), h_);
        }
    }

    LOG_NOTE("Logical output size: %dx%d@%d.%d", w_, h_, x_, y_);
    cursor_x_ = x_ + w_ / 2;
    cursor_y_ = y_ + h_ / 2;

    send_event(EventType::SCREEN_SHAPE_CHANGED, nullptr);
}

void EiScreen::add_device(struct ei_device *device)
{
    LOG_DEBUG("adding device %s", ei_device_get_name(device));

    // Noteworthy: EI in principle supports multiple devices with multiple
    // capabilities, so there may be more than one logical pointer (or even
    // multiple seats). Supporting this is ... tricky so for now we go the easy
    // route: one device for each capability. Note this may be the same device
    // if the first device comes with multiple capabilities.

    if (!ei_pointer_ &&
        ei_device_has_capability(device, EI_DEVICE_CAP_POINTER) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
        ei_pointer_ = ei_device_ref(device);
    }

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
            LOG_WARN("keyboard device %s does not have a keymap, we are guessing", ei_device_get_name(device));
            key_state_->init_default_keymap();
        }
        key_state_->updateKeyMap();
    }

    if (!ei_abs_ &&
        ei_device_has_capability(device, EI_DEVICE_CAP_POINTER_ABSOLUTE) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_BUTTON) &&
        ei_device_has_capability(device, EI_DEVICE_CAP_SCROLL)) {
        ei_abs_ = ei_device_ref(device);
    }

    ei_devices_.emplace_back(ei_device_ref(device));

    update_shape();
}

void EiScreen::remove_device(struct ei_device *device)
{
    LOG_DEBUG("removing device %s", ei_device_get_name(device));

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
    uint32_t button = ei_event_button_get_button(event);

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

bool EiScreen::on_hotkey(KeyID keyid, bool is_pressed, KeyModifierMask mask)
{
    auto it = hotkeys_.find(keyid);

    if (it == hotkeys_.end()) {
        return false;
    }

    // Note: our mask (see on_key_event) only contains some modifiers
    // but we don't put a limitation on modifiers in the hotkeys. So some
    // key combinations may not work correctly, more effort is needed here.
    auto id = it->second.find_by_mask(mask);
    if (id != 0) {
        EventType type = is_pressed ? EventType::PRIMARY_SCREEN_HOTKEY_DOWN : EventType::PRIMARY_SCREEN_HOTKEY_UP;
        events_->add_event(type, get_event_target(), create_event_data<HotKeyInfo>(id));
        return true;
    }

    return false;
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

    LOG_DEBUG1("event: Key %s keycode=%d keyid=%d mask=0x%x", pressed ? "press" : "release", keycode, keyid, mask);

    if (is_primary_ && on_hotkey(keyid, pressed, mask)) {
        return;
    }

    if (keyid != kKeyNone) {
        key_state_->sendKeyEvent(get_event_target(), pressed, false, keyid,
                                 mask, 1, keybutton);
    }
}

void EiScreen::on_button_event(ei_event* event)
{
    LOG_DEBUG("on_button_event");
    assert(is_primary_);

    ButtonID button = map_button_from_evdev(event);
    bool pressed = ei_event_button_get_is_press(event);
    KeyModifierMask mask = key_state_->pollActiveModifiers();

    LOG_DEBUG1("event: Button %s button=%d mask=0x%x", pressed ? "press" : "release", button, mask);

    if (button == kButtonNone) {
        LOG_DEBUG("onButtonEvent: button not recognized");
        return;
    }

    send_event(pressed ? EventType::PRIMARY_SCREEN_BUTTON_DOWN
                       : EventType::PRIMARY_SCREEN_BUTTON_UP,
               create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
}

void EiScreen::on_pointer_scroll_event(ei_event* event)
{
    // Ratio of 10 pixels == one wheel click because that's what mutter/gtk
    // use (for historical reasons).
    const int PIXELS_PER_WHEEL_CLICK = 10;
    // Our logical wheel clicks are multiples 120, so we
    // convert between the two and keep the remainders because
    // we will very likely get subpixel scroll events.
    // This means a single pixel is 120/PIXEL_TO_WHEEL_RATIO in wheel values.
    const int PIXEL_TO_WHEEL_RATIO = 120/PIXELS_PER_WHEEL_CLICK;

    assert(is_primary_);

    double dx = ei_event_scroll_get_dx(event);
    double dy = ei_event_scroll_get_dy(event);
    struct ei_device *device = ei_event_get_device(event);

    LOG_DEBUG1("event: Scroll (%.2f, %.2f)", dx, dy);

    struct ScrollRemainder *remainder = static_cast<struct ScrollRemainder*>(ei_device_get_user_data(device));
    if (!remainder) {
        remainder = new ScrollRemainder();
        ei_device_set_user_data(device, remainder);
    }

    dx += remainder->x;
    dy += remainder->y;

    LOG_DEBUG1("event: after remainder (%.2f, %.2f)", dx, dy);

    double x, y;
    double rx = modf(dx, &x);
    double ry = modf(dy, &y);

    assert(!std::isnan(x) && !std::isinf(x));
    assert(!std::isnan(y) && !std::isinf(y));

    LOG_DEBUG1("event: xy is (%.2f, %.2f)", x, y);

    // libEI and InputLeap seem to use opposite directions, so we have
    // to send the opposite of the value reported by EI if we want to
    // remain compatible with other platforms (including X11).
    if (x != 0 || y != 0)
        send_event(EventType::PRIMARY_SCREEN_WHEEL,
                   create_event_data<WheelInfo>(WheelInfo{(int32_t)-x * PIXEL_TO_WHEEL_RATIO,
                                                          (int32_t)-y * PIXEL_TO_WHEEL_RATIO}));

    remainder->x = rx;
    remainder->y = ry;
    LOG_DEBUG1("event: remainder is (%.2f, %.2f)", x, y);
}

void EiScreen::on_pointer_scroll_discrete_event(ei_event* event)
{
    // both libei and inputleap use multiples of 120 to represent
    // one scroll wheel click event so we can just forward things
    // as-is.

    assert(is_primary_);

    std::int32_t dx = ei_event_scroll_get_discrete_dx(event);
    std::int32_t dy = ei_event_scroll_get_discrete_dy(event);

    LOG_DEBUG1("event: Scroll discrete (%d, %d)", dx, dy);

    // libEI and InputLeap seem to use opposite directions, so we have
    // to send the opposite of the value reported by EI if we want to
    // remain compatible with other platforms (including X11).
    send_event(EventType::PRIMARY_SCREEN_WHEEL,
               create_event_data<WheelInfo>(WheelInfo{-dx, -dy}));
}

void EiScreen::on_motion_event(ei_event* event)
{
    LOG_DEBUG("on_motion_event");
    assert(is_primary_);

    double dx = ei_event_pointer_get_dx(event);
    double dy = ei_event_pointer_get_dy(event);

    if (is_on_screen_) {
        LOG_DEBUG("on_motion_event on primary at (cursor_x_,cursor_y_)=(%i,%i)",
             cursor_x_, cursor_y_);
        send_event(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY,
                   create_event_data<MotionInfo>(MotionInfo{cursor_x_, cursor_y_}));

#if HAVE_LIBPORTAL_INPUTCAPTURE
         if (portal_input_capture_->is_active()) {
             portal_input_capture_->release();
         }
#endif
    } else {
        buffer_dx += dx;
        buffer_dy += dy;
        auto pixel_dx = static_cast<std::int32_t>(buffer_dx);
        auto pixel_dy = static_cast<std::int32_t>(buffer_dy);
        LOG_DEBUG2("on_motion_event(buffer) on secondary at (dx,dy)=(%0.2f,%0.2f)", buffer_dx, buffer_dy);
        if (pixel_dx || pixel_dy) {
            LOG_DEBUG("on_motion_event on secondary at (dx,dy)=(%d,%d)", pixel_dx, pixel_dy);
            send_event(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY,
                       create_event_data<MotionInfo>(MotionInfo{pixel_dx, pixel_dy}));
            buffer_dx -= pixel_dx;
            buffer_dy -= pixel_dy;
        }
    }
}

void EiScreen::on_abs_motion_event(ei_event* event)
{
    assert(is_primary_);
}

void EiScreen::handle_connected_to_eis_event(const Event& event)
{
    int fd = event.get_data_as<int>();
    LOG_DEBUG("We have an EIS connection! fd is %d", fd);

    auto rc = ei_setup_backend_fd(ei_, fd);
    if (rc != 0) {
        LOG_NOTE("Failed to set up ei: %s", strerror(-rc));
    }
}

void EiScreen::handle_portal_session_closed(const Event &event)
{
    // Portal may or may EI_EVENT_DISCONNECT us before sending the DBus Closed signal
    // Let's clean up either way.
    cleanup_ei();
    init_ei();
}

void EiScreen::handle_system_event(const Event& sysevent)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool disconnected = false;

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
                LOG_DEBUG("connected to EIS");
                break;
            case EI_EVENT_SEAT_ADDED:
                if (!ei_seat_) {
                    ei_seat_ = ei_seat_ref(seat);
                    ei_seat_bind_capabilities(ei_seat_, EI_DEVICE_CAP_POINTER,
                                              EI_DEVICE_CAP_POINTER_ABSOLUTE,
                                              EI_DEVICE_CAP_KEYBOARD,
                                              EI_DEVICE_CAP_BUTTON,
                                              EI_DEVICE_CAP_SCROLL,
                                              nullptr);
                    LOG_DEBUG("using seat %s", ei_seat_get_name(ei_seat_));
                    // we don't care about touch
                }
                break;
            case EI_EVENT_DEVICE_ADDED:
                if (seat == ei_seat_) {
                    add_device(device);
                } else {
                    LOG_INFO("seat %s is ignored", ei_seat_get_name(ei_seat_));
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
                // We're using libei which emulates the various seat/device remove events
                // so by the time we get here our EiScreen should be in a neutral state.
                //
                // We don't do anything here, we let the portal's Session.Closed signal
                // handle the rest.
                LOG_WARN("disconnected from EIS");
                disconnected = true;
                break;
            case EI_EVENT_DEVICE_PAUSED:
                LOG_DEBUG("device %s is paused", ei_device_get_name(device));
                break;
            case EI_EVENT_DEVICE_RESUMED:
                LOG_DEBUG("device %s is resumed", ei_device_get_name(device));
                if (!is_primary_ && is_on_screen_) {
                    ei_device_start_emulating(device, ++sequence_number_);
                }
                break;
            case EI_EVENT_KEYBOARD_MODIFIERS:
                // FIXME
                break;

            // events below are for a receiver context (barriers)
            case EI_EVENT_FRAME:
                break;
            case EI_EVENT_DEVICE_START_EMULATING:
                LOG_DEBUG("device %s starts emulating", ei_device_get_name(device));
                break;
            case EI_EVENT_DEVICE_STOP_EMULATING:
                LOG_DEBUG("device %s stops emulating", ei_device_get_name(device));
                break;
            case EI_EVENT_KEYBOARD_KEY:
                on_key_event(event);
                break;
            case EI_EVENT_BUTTON_BUTTON:
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
            case EI_EVENT_SCROLL_DELTA:
                on_pointer_scroll_event(event);
                break;
            case EI_EVENT_SCROLL_DISCRETE:
                on_pointer_scroll_discrete_event(event);
                break;
            case EI_EVENT_SCROLL_STOP:
            case EI_EVENT_SCROLL_CANCEL:
                break;
        }
        ei_event_unref(event);
    }

    if (disconnected)
        ei_ = ei_unref(ei_);
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

EiScreen::HotKeyItem::HotKeyItem(std::uint32_t mask, std::uint32_t id) :
    mask_(mask),
    id_(id)
{
}

EiScreen::HotKeySet::HotKeySet(KeyID key) :
    id_(key)
{
}

bool EiScreen::HotKeySet::remove_by_id(std::uint32_t id)
{
    for (auto it = set_.begin(); it != set_.end(); ++it) {
        if (it->id_ == id) {
            set_.erase(it);
            return true;
        }
    }
    return false;
}

void EiScreen::HotKeySet::add_item(HotKeyItem item)
{
    set_.push_back(item);
}

std::uint32_t EiScreen::HotKeySet::find_by_mask(std::uint32_t mask) const
{
    for (const auto& item : set_) {
        if (item.mask_ == mask) {
            return item.id_;
        }
    }
    return 0;
}

} // namespace inputleap
