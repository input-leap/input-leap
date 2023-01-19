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

#ifndef INPUTLEAP_LIB_SERVER_ICLIENT_CONNECTION_H
#define INPUTLEAP_LIB_SERVER_ICLIENT_CONNECTION_H

#include "inputleap/Fwd.h"
#include "inputleap/clipboard_types.h"
#include "inputleap/key_types.h"
#include "inputleap/mouse_types.h"
#include "inputleap/option_types.h"
#include <string>

namespace inputleap {

class IStream;

/// A low-level interface to write protocol messages
class IClientConnection {
public:
    virtual ~IClientConnection() = default;

    virtual const void* get_event_target() = 0;

    virtual IStream* get_stream() = 0;

    virtual void send_query_info_1_6() = 0;
    virtual void send_leave_1_6() = 0;
    virtual void send_enter_1_6(std::int32_t x_abs, std::int32_t y_abs, std::uint32_t seq_num,
                                KeyModifierMask mask) = 0;
    virtual void send_key_down_1_6(KeyID key, KeyModifierMask mask, KeyButton button) = 0;
    virtual void send_key_up_1_6(KeyID key, KeyModifierMask mask, KeyButton button) = 0;
    virtual void send_key_repeat_1_6(KeyID key, KeyModifierMask mask, std::int32_t count,
                                     KeyButton button) = 0;
    virtual void send_mouse_down_1_6(ButtonID button) = 0;
    virtual void send_mouse_up_1_6(ButtonID button) = 0;
    virtual void send_mouse_move_1_6(std::int32_t x_abs, std::int32_t y_abs) = 0;
    virtual void send_mouse_relative_move_1_6(std::int32_t x_rel, std::int32_t y_rel) = 0;
    virtual void send_mouse_wheel_1_6(std::int32_t x_delta, std::int32_t y_delta) = 0;
    virtual void send_drag_info_1_6(std::uint32_t file_count, const std::string& data) = 0;
    virtual void send_screensaver_1_6(bool on) = 0;
    virtual void send_reset_options_1_6() = 0;
    virtual void send_set_options_1_6(const OptionsList& options) = 0;
    virtual void send_info_ack_1_6() = 0;
    virtual void send_keep_alive_1_6() = 0;
    virtual void send_close_1_6(const char* msg) = 0;

    virtual void send_clipboard_chunk_1_6(const ClipboardChunk& chunk) = 0;
    virtual void send_file_chunk_1_6(const FileChunk& chunk) = 0;
    virtual void send_grab_clipboard(ClipboardID id) = 0;

    virtual void flush() = 0;
    virtual void close() = 0;
};

} // namespace inputleap

#endif // INPUTLEAP_LIB_SERVER_ICLIENT_CONNECTION_H
