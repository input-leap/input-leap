/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2013-2016 Symless Ltd.
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

#pragma once

#include <cstdint>

namespace inputleap {

enum class EventType : std::uint32_t {
    /** An unknown event type. This type is used as a placeholder for unknown events when
        filtering events.
    */
    UNKNOWN,

    /// Exit has been requested.
    QUIT,

    /// This event is sent when system event occurs. The data an instance of system event type
    SYSTEM,

    /// This event is sent when a timer event occurs. The data is pointer to TimerInfo.
    TIMER,

    /// This event is sent when the client has successfully connected to the server.
    CLIENT_CONNECTED,

    /** This event is sent when the server fails for some reason.
         The event data an instance of FailInfo.
    */
    CLIENT_CONNECTION_FAILED,

    /** This event is sent when the client has disconnected from the server (and only after having
        successfully connected).
    */
    CLIENT_DISCONNECTED,

    /// A stream sends this event when \c read() will return with data.
    STREAM_INPUT_READY,

    /** A stream sends this event when the output buffer has been flushed. If there have been no
        writes since the event was posted, calling \c shutdownOutput() or close() will not discard
        any data and \c flush() will return immediately.
    */
    STREAM_OUTPUT_FLUSHED,

    /// A stream sends this event when a write has failed.
    STREAM_OUTPUT_ERROR,

    /** This event is sent when the input side of the stream has shutdown. When the input has
        shutdown, no more data will ever be available to read.
    */
    STREAM_INPUT_SHUTDOWN,

    /** This event is sent when the output side of the stream has shutdown. When the output has
        shutdown, no more data can ever be written to the stream. Any attempt to do so will
        generate a output error event.
    */
    STREAM_OUTPUT_SHUTDOWN,

    /// This event is sent when a stream receives an irrecoverable input format error.
    STREAM_INPUT_FORMAT_ERROR,

    /// This event is sent when the socket is connected.
    IPC_CLIENT_CONNECTED,

    /// This event is sent when a message is received.
    IPC_CLIENT_MESSAGE_RECEIVED,

    /// This event is sent when the server receives a message from a client.
    IPC_CLIENT_PROXY_MESSAGE_RECEIVED,

    /// This event is sent when the client disconnects from the server.
    IPC_CLIENT_PROXY_DISCONNECTED,

    /** This event is sent when we have created the client proxy.
        Event data is a pointer to IpcClientProxy.
    */
    IPC_SERVER_CLIENT_CONNECTED,

    /// This event is sent when a message is received through a client proxy.
    IPC_SERVER_MESSAGE_RECEIVED,

    /// This event is sent when the client receives a message from the server.
    IPC_SERVER_PROXY_MESSAGE_RECEIVED,

    /// A socket sends this event when a remote connection has been established.
    DATA_SOCKET_CONNECTED,

    /// A secure socket sends this event when a remote connection has been established.
    DATA_SOCKET_SECURE_CONNECTED,

    /** A socket sends this event when an attempt to connect to a remote port has failed.
        The data an instance of a ConnectionFailedInfo.
    */
    DATA_SOCKET_CONNECTION_FAILED,

    /// A socket sends this event when a remote connection is waiting to be accepted.
    LISTEN_SOCKET_CONNECTING,

    /** A socket sends this event when the remote side of the socket has disconnected or
        shutdown both input and output.
    */
    SOCKET_DISCONNECTED,

    /// This is sent when the client doesn't want to reconnect after it disconnects from the server.
    SOCKET_STOP_RETRY,

    OSX_SCREEN_CONFIRM_SLEEP,

    /// This event is sent whenever a server accepts a client.
    CLIENT_LISTENER_ACCEPTED,

    /// This event is sent whenever a client connects.
    CLIENT_LISTENER_CONNECTED,

    /** This event is sent when the client has completed the initial handshake.  Until it is sent,
        the client is not fully connected.
    */
    CLIENT_PROXY_READY,

    /** This event is sent when the client disconnects or is disconnected. The target is
        getEventTarget().
    */
    CLIENT_PROXY_DISCONNECTED,

    /** This event is sent when the client has correctly responded to the hello message.
        The target is this.
    */
    CLIENT_PROXY_UNKNOWN_SUCCESS,

    /** This event is sent when a client fails to correctly respond to the hello message.
        The target is this.
    */
    CLIENT_PROXY_UNKNOWN_FAILURE,

    /// This event is sent when the server fails for some reason.
    SERVER_ERROR,

    /** This event is sent when a client screen has connected.
        The event data an instance of ScreenConnectedInfo that indicates the connected screen.
    */
    SERVER_CONNECTED,

    /// This is event sent when all the clients have disconnected.
    SERVER_DISCONNECTED,

    /** This event is sent to inform the server to switch screens.
        The event data an instance of SwitchToScreenInfo that indicates the target screen.
    */
    SERVER_SWITCH_TO_SCREEN,

    /// This event is sent to inform the server to toggle screens.  These is no event data.
    SERVER_TOGGLE_SCREEN,

    /** This event is sent to inform the server to switch screens.
        The event data an instance of SwitchInDirectionInfo that indicates the target direction.
    */
    SERVER_SWITCH_INDIRECTION,

    /** This event is sent to inform the server to turn keyboard broadcasting on or off.
        The event data an instance of KeyboardBroadcastInfo.
    */
    SERVER_KEYBOARD_BROADCAST,

    /** This event is sent to inform the server to lock the cursor to the active screen or to
        unlock it. The event data an instance of LockCursorToScreenInfo.
    */
    SERVER_LOCK_CURSOR_TO_SCREEN,

    /// This event is sent when the screen has been switched to a client.
    SERVER_SCREEN_SWITCHED,

    SERVER_APP_RELOAD_CONFIG,
    SERVER_APP_FORCE_RECONNECT,
    SERVER_APP_RESET_SERVER,

    /// This event is sent when key is down. Event data is an instance of KeyInfo (count == 1)
    KEY_STATE_KEY_DOWN,
    /// This event is sent when key is up. Event data is an instance of KeyInfo (count == 1)
    KEY_STATE_KEY_UP,
    /// This event is sent when key is repeated. Event data is an instance of KeyInfo.
    KEY_STATE_KEY_REPEAT,

    /// This event is sent when button is down. Event data is an instance of ButtonInfo
    PRIMARY_SCREEN_BUTTON_DOWN,

    /// This event is sent when button is up. Event data is an instance of ButtonInfo
    PRIMARY_SCREEN_BUTTON_UP,

    /** This event is sent when mouse moves on primary screen.
        Event data an instance of MotionInfo, the values are absolute position.
    */
    PRIMARY_SCREEN_MOTION_ON_PRIMARY,

    /** This event is sent when mouse moves on secondary screen.
        Event data an instance of MotionInfo, the values are relative motion deltas.
    */
    PRIMARY_SCREEN_MOTION_ON_SECONDARY,

    /// This event is sent when mouse wheel is rotated. Event data an instance of WheelInfo.
    PRIMARY_SCREEN_WHEEL,

    /// This event is sent when screensaver is activated.
    PRIMARY_SCREEN_SAVER_ACTIVATED,

    /// This event is sent when screensaver is deactivated.
    PRIMARY_SCREEN_SAVER_DEACTIVATED,

    /// This event is sent when hotkey is down. Event data an instance of HotKeyInfo.
    PRIMARY_SCREEN_HOTKEY_DOWN,

    /// This event is sent when hotkey is up. Event data an instance of HotKeyInfo.
    PRIMARY_SCREEN_HOTKEY_UP,

    /// This event is sent when fake input begins.
    PRIMARY_SCREEN_FAKE_INPUT_BEGIN,

    /// This event is sent when fake input ends.
    PRIMARY_SCREEN_FAKE_INPUT_END,

    /** This event is sent whenever the screen has failed for some reason (e.g. the X Windows
        server died).
    */
    SCREEN_ERROR,

    /// This event is sent whenever the screen's shape changes.
    SCREEN_SHAPE_CHANGED,

    /** This event is sent whenever the system goes to sleep or a user session is deactivated (fast
        user switching).
    */
    SCREEN_SUSPEND,

    /** This event is sent whenever the system wakes up or a user session is activated (fast user
        switching).
    */
    SCREEN_RESUME,

    /** This event is sent whenever the clipboard is grabbed by some other application so we
        don't own it anymore. The data an instance of a ClipboardInfo.
    */
    CLIPBOARD_GRABBED,

    /** This event is sent whenever the contents of the clipboard has changed.
        The data an instance of a ClipboardInfo.
    */
    CLIPBOARD_CHANGED,

    /// This event is sent whenever a clipboard chunk is transferred.
    CLIPBOARD_SENDING,

    /// This event is sent whenever a file chunk is transferred.
    FILE_CHUNK_SENDING,

    /// This event is sent whenever file has been received.
    FILE_RECEIVE_COMPLETED,

    /// This event is a keepalive event.
    FILE_KEEPALIVE,

    /// The total number of known event types.
    EVENT_COUNT,
};

} // namespace inputleap
