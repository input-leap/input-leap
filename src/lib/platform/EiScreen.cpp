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

EiScreen::EiScreen(bool is_primary, IEventQueue* events) :
    is_primary_(is_primary),
    events_(events)
{
    // Server isn't supported yet
    assert(!is_primary);

    ei_ = ei_new(NULL);
    ei_log_set_priority(ei_, EI_LOG_PRIORITY_DEBUG);
    ei_configure_name(ei_, "InputLeap client");
    auto rc = ei_setup_backend_socket(ei_, NULL);
    if (rc != 0) {
        LOG((CLOG_DEBUG "ei init error: %s", strerror(-rc)));
        throw std::runtime_error("Failed to init ei context");
    }

    key_state_ = new EiKeyState(this, events);
    // install event handlers
    events_->add_handler(EventType::SYSTEM, events_->getSystemTarget(),
                         [this](const auto& e){ handle_system_event(e); });

    // install the platform event queue
    events_->set_buffer(std::make_unique<EiEventQueueBuffer>(ei_, events_));
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
    // FIXME
    assert(!"Not Implemented");
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
    // FIXME
}

void EiScreen::fakeKey(uint32_t keycode, bool is_down) const
{
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
            case EI_EVENT_DEVICE_START_EMULATING:
            case EI_EVENT_DEVICE_STOP_EMULATING:
            case EI_EVENT_KEYBOARD_KEY:
            case EI_EVENT_POINTER_BUTTON:
            case EI_EVENT_POINTER_MOTION:
            case EI_EVENT_POINTER_MOTION_ABSOLUTE:
            case EI_EVENT_TOUCH_UP:
            case EI_EVENT_TOUCH_MOTION:
            case EI_EVENT_TOUCH_DOWN:
            case EI_EVENT_POINTER_SCROLL:
            case EI_EVENT_POINTER_SCROLL_DISCRETE:
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