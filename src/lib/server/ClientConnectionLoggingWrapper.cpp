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

#include "ClientConnectionLoggingWrapper.h"
#include "base/Log.h"
#include "inputleap/FileChunk.h"
#include "inputleap/ProtocolUtil.h"
#include "inputleap/protocol_types.h"
#include "io/IStream.h"

namespace inputleap {

ClientConnectionLoggingWrapper::ClientConnectionLoggingWrapper(
        const std::string& name, std::unique_ptr<IClientConnection> conn) :
    name_{name},
    conn_{std::move(conn)}
{}

ClientConnectionLoggingWrapper::~ClientConnectionLoggingWrapper() = default;

void* ClientConnectionLoggingWrapper::get_event_target()
{
    return conn_->get_event_target();
}

IStream* ClientConnectionLoggingWrapper::get_stream()
{
    return conn_->get_stream();
}

void ClientConnectionLoggingWrapper::send_query_info_1_6()
{
    LOG((CLOG_DEBUG1 "querying client \"%s\" info", name_.c_str()));
    conn_->send_query_info_1_6();
}

void ClientConnectionLoggingWrapper::send_enter_1_6(std::int32_t x_abs, std::int32_t y_abs,
                                                    std::uint32_t seq_num, KeyModifierMask mask)
{
    LOG((CLOG_DEBUG1 "send enter to \"%s\", %d,%d %d %04x", name_.c_str(), x_abs, y_abs,
         seq_num, mask));
    conn_->send_enter_1_6(x_abs, y_abs, seq_num, mask);
}

void ClientConnectionLoggingWrapper::send_leave_1_6()
{
    LOG((CLOG_DEBUG1 "send leave to \"%s\"", name_.c_str()));
    conn_->send_leave_1_6();
}

void ClientConnectionLoggingWrapper::send_key_down_1_6(KeyID key, KeyModifierMask mask,
                                                       KeyButton button)
{
    LOG((CLOG_DEBUG1 "send key down to \"%s\" id=%d, mask=0x%04x, button=0x%04x",
         name_.c_str(), key, mask, button));
    conn_->send_key_down_1_6(key, mask, button);
}

void ClientConnectionLoggingWrapper::send_key_up_1_6(KeyID key, KeyModifierMask mask,
                                                     KeyButton button)
{
    LOG((CLOG_DEBUG1 "send key up to \"%s\" id=%d, mask=0x%04x, button=0x%04x",
         name_.c_str(), key, mask, button));
    conn_->send_key_up_1_6(key, mask, button);
}

void ClientConnectionLoggingWrapper::send_key_repeat_1_6(KeyID key, KeyModifierMask mask,
                                                         std::int32_t count, KeyButton button)
{
    LOG((CLOG_DEBUG1 "send key repeat to \"%s\" id=%d, mask=0x%04x, count=%d, button=0x%04x",
         name_.c_str(), key, mask, count, button));
    conn_->send_key_repeat_1_6(key, mask, count, button);
}

void ClientConnectionLoggingWrapper::send_mouse_down_1_6(ButtonID button)
{
    LOG((CLOG_DEBUG1 "send mouse down to \"%s\" id=%d", name_.c_str(), button));
    conn_->send_mouse_down_1_6(button);
}

void ClientConnectionLoggingWrapper::send_mouse_up_1_6(ButtonID button)
{
    LOG((CLOG_DEBUG1 "send mouse up to \"%s\" id=%d", name_.c_str(), button));
    conn_->send_mouse_up_1_6(button);
}

void ClientConnectionLoggingWrapper::send_mouse_move_1_6(std::int32_t x_abs, std::int32_t y_abs)
{
    LOG((CLOG_DEBUG2 "send mouse move to \"%s\" %d,%d", name_.c_str(), x_abs, y_abs));
    conn_->send_mouse_move_1_6(x_abs, y_abs);
}

void ClientConnectionLoggingWrapper::send_mouse_relative_move_1_6(std::int32_t x_rel,
                                                                  std::int32_t y_rel)
{
    LOG((CLOG_DEBUG2 "send mouse relative move to \"%s\" %d,%d", name_.c_str(), x_rel, y_rel));
    conn_->send_mouse_relative_move_1_6(x_rel, y_rel);
}

void ClientConnectionLoggingWrapper::send_mouse_wheel_1_6(std::int32_t x_delta, std::int32_t y_delta)
{
    LOG((CLOG_DEBUG2 "send mouse wheel to \"%s\" %+d,%+d", name_.c_str(), x_delta, y_delta));
    conn_->send_mouse_wheel_1_6(x_delta, y_delta);
}

void ClientConnectionLoggingWrapper::send_drag_info_1_6(std::uint32_t file_count,
                                                        const std::string& data)
{
    LOG((CLOG_DEBUG2 "send drag info to \"%s\" %d, %d", name_.c_str(), file_count, data.size()));
    conn_->send_drag_info_1_6(file_count, data);
}

void ClientConnectionLoggingWrapper::send_screensaver_1_6(bool on)
{
    LOG((CLOG_DEBUG1 "send screen saver to \"%s\" on=%d", name_.c_str(), on ? 1 : 0));
    conn_->send_screensaver_1_6(on);
}

void ClientConnectionLoggingWrapper::send_reset_options_1_6()
{
    LOG((CLOG_DEBUG1 "send reset options to \"%s\"", name_.c_str()));
    conn_->send_reset_options_1_6();
}

void ClientConnectionLoggingWrapper::send_set_options_1_6(const OptionsList& options)
{
    LOG((CLOG_DEBUG1 "send set options to \"%s\" size=%d", name_.c_str(), options.size()));
    conn_->send_set_options_1_6(options);
}

void ClientConnectionLoggingWrapper::send_info_ack_1_6()
{
    LOG((CLOG_DEBUG1 "send info ack to \"%s\"", name_.c_str()));
    conn_->send_info_ack_1_6();
}

void ClientConnectionLoggingWrapper::send_keep_alive_1_6()
{
    conn_->send_keep_alive_1_6();
}

void ClientConnectionLoggingWrapper::send_close_1_6(const char* msg)
{
    LOG((CLOG_DEBUG1 "send close \"%s\" to \"%s\"", msg, name_.c_str()));
    conn_->send_close_1_6(msg);
}

void ClientConnectionLoggingWrapper::send_file_chunk_1_6(const FileChunk& chunk)
{
    switch (chunk.mark_) {
    case kDataStart:
        LOG((CLOG_DEBUG2 "sending file chunk start: size=%s", chunk.data_.c_str()));
        break;

    case kDataChunk:
        LOG((CLOG_DEBUG2 "sending file chunk: size=%i", chunk.data_.size()));
        break;

    case kDataEnd:
        LOG((CLOG_DEBUG2 "sending file finished"));
        break;
    default:
        break;
    }
    conn_->send_file_chunk_1_6(chunk);
}

void ClientConnectionLoggingWrapper::send_grab_clipboard(ClipboardID id)
{
    LOG((CLOG_DEBUG "send grab clipboard %d to \"%s\"", id, name_.c_str()));
    conn_->send_grab_clipboard(id);
}

void ClientConnectionLoggingWrapper::flush()
{
    conn_->flush();
}

void ClientConnectionLoggingWrapper::close()
{
    conn_->close();
}

} // namespace inputleap
