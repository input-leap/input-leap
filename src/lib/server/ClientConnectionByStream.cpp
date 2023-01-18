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

#include "ClientConnectionByStream.h"
#include "base/Log.h"
#include "inputleap/ProtocolUtil.h"
#include "inputleap/protocol_types.h"
#include "io/IStream.h"

namespace inputleap {

ClientConnectionByStream::ClientConnectionByStream(std::unique_ptr<IStream> stream) :
    stream_{std::move(stream)}
{}

ClientConnectionByStream::~ClientConnectionByStream() = default;

void* ClientConnectionByStream::get_event_target()
{
    return stream_->getEventTarget();
}

void ClientConnectionByStream::send_query_info_1_6()
{
    ProtocolUtil::writef(stream_.get(), kMsgQInfo);
}

void ClientConnectionByStream::send_enter_1_6(std::int32_t x_abs, std::int32_t y_abs,
                                              std::uint32_t seq_num, KeyModifierMask mask)
{
    ProtocolUtil::writef(stream_.get(), kMsgCEnter, x_abs, y_abs, seq_num, mask);
}

void ClientConnectionByStream::send_leave_1_6()
{
    ProtocolUtil::writef(stream_.get(), kMsgCLeave);
}

void ClientConnectionByStream::send_key_down_1_6(KeyID key, KeyModifierMask mask, KeyButton button)
{
    ProtocolUtil::writef(stream_.get(), kMsgDKeyDown, key, mask, button);
}

void ClientConnectionByStream::send_key_up_1_6(KeyID key, KeyModifierMask mask, KeyButton button)
{
    ProtocolUtil::writef(stream_.get(), kMsgDKeyUp, key, mask, button);
}

void ClientConnectionByStream::send_key_repeat_1_6(KeyID key, KeyModifierMask mask,
                                                   std::int32_t count, KeyButton button)
{
    ProtocolUtil::writef(stream_.get(), kMsgDKeyRepeat, key, mask, count, button);
}

void ClientConnectionByStream::send_mouse_down_1_6(ButtonID button)
{
    ProtocolUtil::writef(stream_.get(), kMsgDMouseDown, button);
}

void ClientConnectionByStream::send_mouse_up_1_6(ButtonID button)
{
    ProtocolUtil::writef(stream_.get(), kMsgDMouseUp, button);
}

void ClientConnectionByStream::send_mouse_move_1_6(std::int32_t x_abs, std::int32_t y_abs)
{
    ProtocolUtil::writef(stream_.get(), kMsgDMouseMove, x_abs, y_abs);
}

void ClientConnectionByStream::send_mouse_relative_move_1_6(std::int32_t x_rel, std::int32_t y_rel)
{
    ProtocolUtil::writef(stream_.get(), kMsgDMouseRelMove, x_rel, y_rel);
}

void ClientConnectionByStream::send_mouse_wheel_1_6(std::int32_t x_delta, std::int32_t y_delta)
{
    ProtocolUtil::writef(stream_.get(), kMsgDMouseWheel, x_delta, y_delta);
}

void ClientConnectionByStream::send_drag_info_1_6(std::uint32_t file_count, const std::string& data)
{
    ProtocolUtil::writef(stream_.get(), kMsgDDragInfo, file_count, &data);
}

void ClientConnectionByStream::send_screensaver_1_6(bool on)
{
    ProtocolUtil::writef(stream_.get(), kMsgCScreenSaver, on ? 1 : 0);
}

void ClientConnectionByStream::send_reset_options_1_6()
{
    ProtocolUtil::writef(stream_.get(), kMsgCResetOptions);
}

void ClientConnectionByStream::send_set_options_1_6(const OptionsList& options)
{
    ProtocolUtil::writef(stream_.get(), kMsgDSetOptions, &options);
}

void ClientConnectionByStream::send_info_ack_1_6()
{
    ProtocolUtil::writef(stream_.get(), kMsgCInfoAck);
}

void ClientConnectionByStream::send_keep_alive_1_6()
{
    ProtocolUtil::writef(stream_.get(), kMsgCKeepAlive);
}

void ClientConnectionByStream::send_close_1_6(const char* msg)
{
    ProtocolUtil::writef(stream_.get(), msg);
}

void ClientConnectionByStream::send_grab_clipboard(ClipboardID id)
{
    ProtocolUtil::writef(stream_.get(), kMsgCClipboard, id, 0);
}

void ClientConnectionByStream::flush()
{
    stream_->flush();
}

void ClientConnectionByStream::close()
{
    stream_->close();
}

} // namespace inputleap
