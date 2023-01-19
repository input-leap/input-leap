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


#ifndef INPUTLEAP_LIB_SERVER_CLIENT_CONNECTION_LOGGING_WRAPPER_H
#define INPUTLEAP_LIB_SERVER_CLIENT_CONNECTION_LOGGING_WRAPPER_H

#include "IClientConnection.h"
#include <memory>

namespace inputleap {

class IStream;

/// Wraps a IClientConnection and logs messages received from and sent to it.
class ClientConnectionLoggingWrapper : public IClientConnection {
public:
    ClientConnectionLoggingWrapper(const std::string& name,
                                   std::unique_ptr<IClientConnection> conn);
    ~ClientConnectionLoggingWrapper() override;

    IStream* get_stream() override;

    const EventTarget* get_event_target() override;

    void send_query_info_1_6() override;
    void send_enter_1_6(std::int32_t x_abs, std::int32_t y_abs, std::uint32_t seq_num,
                        KeyModifierMask mask) override;
    void send_leave_1_6() override;
    void send_key_down_1_6(KeyID key, KeyModifierMask mask, KeyButton button) override;
    void send_key_up_1_6(KeyID key, KeyModifierMask mask, KeyButton button) override;
    void send_key_repeat_1_6(KeyID key, KeyModifierMask mask, std::int32_t count,
                             KeyButton button) override;
    void send_mouse_down_1_6(ButtonID button) override;
    void send_mouse_up_1_6(ButtonID button) override;
    void send_mouse_move_1_6(std::int32_t x_abs, std::int32_t y_abs) override;
    void send_mouse_relative_move_1_6(std::int32_t x_rel, std::int32_t y_rel) override;
    void send_mouse_wheel_1_6(std::int32_t x_delta, std::int32_t y_delta) override;
    void send_drag_info_1_6(std::uint32_t file_count, const std::string& data) override;
    void send_screensaver_1_6(bool on) override;
    void send_reset_options_1_6() override;
    void send_set_options_1_6(const OptionsList& options) override;
    void send_info_ack_1_6() override;
    void send_keep_alive_1_6() override;
    void send_close_1_6(const char* msg) override;

    void send_clipboard_chunk_1_6(const ClipboardChunk& chunk) override;
    void send_file_chunk_1_6(const FileChunk& chunk) override;
    void send_grab_clipboard(ClipboardID id) override;

    void flush() override;
    void close() override;

private:
    std::string name_;
    std::unique_ptr<IClientConnection> conn_;
};

} // namespace inputleap

#endif // INPUTLEAP_LIB_SERVER_CLIENT_CONNECTION_LOGGING_WRAPPER_H
