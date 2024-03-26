/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "client/Client.h"

#include "client/ServerProxy.h"
#include "inputleap/Screen.h"
#include "inputleap/FileChunk.h"
#include "inputleap/DropHelper.h"
#include "inputleap/PacketStreamFilter.h"
#include "inputleap/ProtocolUtil.h"
#include "inputleap/protocol_types.h"
#include "inputleap/Exceptions.h"
#include "inputleap/StreamChunker.h"
#include "inputleap/IPlatformScreen.h"
#include "mt/Thread.h"
#include "net/TCPSocket.h"
#include "net/IDataSocket.h"
#include "net/ISocketFactory.h"
#include "net/SecureSocket.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/EventQueueTimer.h"
#include "base/IEventQueue.h"
#include "base/Time.h"


#include <cstring>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <fstream>

namespace inputleap {

Client::Client(IEventQueue* events, const std::string& name, const NetworkAddress& address,
               ISocketFactory* socketFactory,
               inputleap::Screen* screen,
               ClientArgs const& args) :
    m_mock(false),
    m_name(name),
    m_serverAddress(address),
    m_socketFactory(socketFactory),
    m_screen(screen),
    m_stream(nullptr),
    m_timer(nullptr),
    m_server(nullptr),
    m_ready(false),
    m_active(false),
    m_suspended(false),
    m_connectOnResume(false),
    m_events(events),
    m_sendFileThread(nullptr),
    m_writeToDropDirThread(nullptr),
    m_useSecureNetwork(args.m_enableCrypto),
    m_args(args),
    m_enableClipboard(true),
    m_maximumClipboardSize(INT_MAX)
{
    assert(m_socketFactory != nullptr);
    assert(m_screen != nullptr);

    // register suspend/resume event handlers
    m_events->add_handler(EventType::SCREEN_SUSPEND, get_event_target(),
                          [this](const auto& e){ handle_suspend(); });
    m_events->add_handler(EventType::SCREEN_RESUME, get_event_target(),
                          [this](const auto& e){ handle_resume(); });

    if (m_args.m_enableDragDrop) {
        m_events->add_handler(EventType::FILE_CHUNK_SENDING, this,
                              [this](const auto& e){ handle_file_chunk_sending(e); });
        m_events->add_handler(EventType::FILE_RECEIVE_COMPLETED, this,
                              [this](const auto& e){ handle_file_receive_completed(e); });
    }
}

Client::~Client()
{
    if (m_mock) {
        return;
    }

    m_events->remove_handler(EventType::SCREEN_SUSPEND, get_event_target());
    m_events->remove_handler(EventType::SCREEN_RESUME, get_event_target());

    cleanupTimer();
    cleanupScreen();
    cleanupConnecting();
    cleanupConnection();
    delete m_socketFactory;
}

void
Client::connect()
{
    if (m_stream != nullptr) {
        return;
    }
    if (m_suspended) {
        m_connectOnResume = true;
        return;
    }

    auto security_level = ConnectionSecurityLevel::PLAINTEXT;
    if (m_useSecureNetwork) {
        // client always authenticates server
        security_level = ConnectionSecurityLevel::ENCRYPTED_AUTHENTICATED;
    }

    try {
        // resolve the server hostname.  do this every time we connect
        // in case we couldn't resolve the address earlier or the address
        // has changed (which can happen frequently if this is a laptop
        // being shuttled between various networks).  patch by Brent
        // Priddy.
        m_serverAddress.resolve();

        // m_serverAddress will be null if the hostname address is not reolved
        if (m_serverAddress.getAddress() != nullptr) {
          // to help users troubleshoot, show server host name (issue: 60)
          LOG_NOTE("connecting to '%s': %s:%i",
          m_serverAddress.getHostname().c_str(),
          ARCH->addrToString(m_serverAddress.getAddress()).c_str(),
          m_serverAddress.getPort());
        }

        // create the socket
        auto socket = m_socketFactory->create(ARCH->getAddrFamily(m_serverAddress.getAddress()),
                                              security_level);
        // filter socket messages, including a packetizing filter
        auto socket_ptr = socket.get();
        m_stream = new PacketStreamFilter(m_events, std::move(socket));

        // connect
        LOG_DEBUG1("connecting to server");
        setupConnecting();
        setupTimer();
        socket_ptr->connect(m_serverAddress);
    }
    catch (XBase& e) {
        cleanupTimer();
        cleanupConnecting();
        cleanupStream();
        LOG_DEBUG1("connection failed");
        sendConnectionFailedEvent(e.what());
        return;
    }
}

void
Client::disconnect(const char* msg)
{
    m_connectOnResume = false;
    cleanupTimer();
    cleanupScreen();
    cleanupConnecting();
    cleanupConnection();
    if (msg != nullptr) {
        sendConnectionFailedEvent(msg);
    }
    else {
        send_event(EventType::CLIENT_DISCONNECTED);
    }
}

void
Client::handshakeComplete()
{
    m_ready = true;
    m_screen->enable();
    send_event(EventType::CLIENT_CONNECTED);
}

bool
Client::isConnected() const
{
    return (m_server != nullptr);
}

bool
Client::isConnecting() const
{
    return (m_timer != nullptr);
}

NetworkAddress
Client::getServerAddress() const
{
    return m_serverAddress;
}

const EventTarget* Client::get_event_target() const
{
    return m_screen->get_event_target();
}

bool
Client::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    return m_screen->getClipboard(id, clipboard);
}

void Client::getShape(std::int32_t& x, std::int32_t& y, std::int32_t& w, std::int32_t& h) const
{
    m_screen->getShape(x, y, w, h);
}

void Client::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
    m_screen->getCursorPos(x, y);
}

void Client::enter(std::int32_t xAbs, std::int32_t yAbs, std::uint32_t, KeyModifierMask mask, bool)
{
    m_active = true;
    m_screen->mouseMove(xAbs, yAbs);
    m_screen->enter(mask);

    if (m_sendFileThread != nullptr) {
        StreamChunker::interruptFile();
        m_sendFileThread = nullptr;
    }
}

bool
Client::leave()
{
    m_active = false;

    m_screen->leave();

    if (m_enableClipboard) {
        // send clipboards that we own and that have changed
        for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
            if (m_ownClipboard[id]) {
                sendClipboard(id);
            }
        }
    }

    return true;
}

void
Client::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
     m_screen->setClipboard(id, clipboard);
    m_ownClipboard[id]  = false;
    m_sentClipboard[id] = false;
}

void
Client::grabClipboard(ClipboardID id)
{
    m_screen->grabClipboard(id);
    m_ownClipboard[id]  = false;
    m_sentClipboard[id] = false;
}

void
Client::setClipboardDirty(ClipboardID, bool)
{
    assert(0 && "shouldn't be called");
}

void
Client::keyDown(KeyID id, KeyModifierMask mask, KeyButton button)
{
     m_screen->keyDown(id, mask, button);
}

void Client::keyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count, KeyButton button)
{
     m_screen->keyRepeat(id, mask, count, button);
}

void
Client::keyUp(KeyID id, KeyModifierMask mask, KeyButton button)
{
     m_screen->keyUp(id, mask, button);
}

void
Client::mouseDown(ButtonID id)
{
     m_screen->mouseDown(id);
}

void
Client::mouseUp(ButtonID id)
{
     m_screen->mouseUp(id);
}

void Client::mouseMove(std::int32_t x, std::int32_t y)
{
    m_screen->mouseMove(x, y);
}

void Client::mouseRelativeMove(std::int32_t dx, std::int32_t dy)
{
    m_screen->mouseRelativeMove(dx, dy);
}

void Client::mouseWheel(std::int32_t xDelta, std::int32_t yDelta)
{
    m_screen->mouseWheel(xDelta, yDelta);
}

void
Client::screensaver(bool activate)
{
     m_screen->screensaver(activate);
}

void
Client::resetOptions()
{
    m_screen->resetOptions();
}

void
Client::setOptions(const OptionsList& options)
{
    for (OptionsList::const_iterator index = options.begin();
         index != options.end(); ++index) {
        const OptionID id       = *index;
        if (id == kOptionClipboardSharing) {
            index++;
            if (*index == static_cast<OptionValue>(false)) {
                LOG_NOTE("clipboard sharing is disabled");
            }
            m_enableClipboard = *index;

            break;
        } else if (id == kOptionClipboardSharingSize) {
            index++;
            if (index != options.end()) {
                m_maximumClipboardSize = *index;
            }
        }
    }

    if (m_enableClipboard && !m_maximumClipboardSize) {
        m_enableClipboard = false;
        LOG_NOTE("clipboard sharing is disabled because the server "
                       "set the maximum clipboard size to 0");
    }

    m_screen->setOptions(options);
}

std::string
Client::getName() const
{
    return m_name;
}

void
Client::sendClipboard(ClipboardID id)
{
    // note -- m_mutex must be locked on entry
    assert(m_screen != nullptr);
    assert(m_server != nullptr);

    // get clipboard data.  set the clipboard time to the last
    // clipboard time before getting the data from the screen
    // as the screen may detect an unchanged clipboard and
    // avoid copying the data.
    Clipboard clipboard;
    if (clipboard.open(m_timeClipboard[id])) {
        clipboard.close();
    }
    m_screen->getClipboard(id, &clipboard);

    // check time
    if (m_timeClipboard[id] == 0 ||
        clipboard.getTime() != m_timeClipboard[id]) {
        // save new time
        m_timeClipboard[id] = clipboard.getTime();

        // marshall the data
        std::string data = clipboard.marshall();
        if (data.size() >= m_maximumClipboardSize) {
            LOG_NOTE("Skipping clipboard transfer because the clipboard"
                " contents exceeds the %zi MB size limit set by the server",
                m_maximumClipboardSize);
            return;
        }

        // save and send data if different or not yet sent
        if (!m_sentClipboard[id] || data != m_dataClipboard[id]) {
            m_sentClipboard[id] = true;
            m_dataClipboard[id] = data;
            m_server->onClipboardChanged(id, &clipboard);
        }
    }
}

void Client::send_event(EventType type)
{
    m_events->add_event(type, get_event_target());
}

void
Client::sendConnectionFailedEvent(const char* msg)
{
    FailInfo info{msg};
    info.m_retry = true;
    m_events->add_event(EventType::CLIENT_CONNECTION_FAILED, get_event_target(),
                        create_event_data<FailInfo>(info));
}

void Client::send_file_chunk(const FileChunk& chunk)
{
    LOG_DEBUG1("send file chunk");
    assert(m_server != nullptr);

    m_server->file_chunk_sending(chunk);
}

void
Client::setupConnecting()
{
    assert(m_stream != nullptr);

    if (m_args.m_enableCrypto) {
        m_events->add_handler(EventType::DATA_SOCKET_SECURE_CONNECTED, m_stream->get_event_target(),
                              [this](const auto& e){ handle_connected(); });
    }
    else {
        m_events->add_handler(EventType::DATA_SOCKET_CONNECTED, m_stream->get_event_target(),
                              [this](const auto& e){ handle_connected(); });
    }
    m_events->add_handler(EventType::DATA_SOCKET_CONNECTION_FAILED, m_stream->get_event_target(),
                          [this](const auto& e){ handle_connection_failed(e); });
}

void
Client::setupConnection()
{
    assert(m_stream != nullptr);

    m_events->add_handler(EventType::SOCKET_DISCONNECTED, m_stream->get_event_target(),
                          [this](const auto& e){ handle_disconnected(); });
    m_events->add_handler(EventType::STREAM_INPUT_READY, m_stream->get_event_target(),
                          [this](const auto& e){ handle_hello(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_ERROR, m_stream->get_event_target(),
                          [this](const auto& e){ handle_output_error(); });
    m_events->add_handler(EventType::STREAM_INPUT_SHUTDOWN, m_stream->get_event_target(),
                          [this](const auto& e){ handle_disconnected(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_SHUTDOWN, m_stream->get_event_target(),
                          [this](const auto& e){ handle_disconnected(); });
    m_events->add_handler(EventType::SOCKET_STOP_RETRY, m_stream->get_event_target(),
                          [this](const auto& e){ handle_stop_retry(); });
}

void
Client::setupScreen()
{
    assert(m_server == nullptr);

    m_ready  = false;
    m_server = new ServerProxy(this, m_stream, m_events);
    m_events->add_handler(EventType::SCREEN_SHAPE_CHANGED, get_event_target(),
                          [this](const auto& e){ handle_shape_changed(); });
    m_events->add_handler(EventType::CLIPBOARD_GRABBED, get_event_target(),
                          [this](const auto& e){ handle_clipboard_grabbed(e); });
}

void
Client::setupTimer()
{
    assert(m_timer == nullptr);

    m_timer = m_events->newOneShotTimer(15.0, nullptr);
    m_events->add_handler(EventType::TIMER, m_timer,
                          [this](const auto& e){ handle_connect_timeout(); });
}

void
Client::cleanupConnecting()
{
    if (m_stream != nullptr) {
        m_events->remove_handler(EventType::DATA_SOCKET_CONNECTED, m_stream->get_event_target());
        m_events->remove_handler(EventType::DATA_SOCKET_CONNECTION_FAILED,
                                m_stream->get_event_target());
    }
}

void
Client::cleanupConnection()
{
    if (m_stream != nullptr) {
        m_events->remove_handler(EventType::STREAM_INPUT_READY, m_stream->get_event_target());
        m_events->remove_handler(EventType::STREAM_OUTPUT_ERROR, m_stream->get_event_target());
        m_events->remove_handler(EventType::STREAM_INPUT_SHUTDOWN, m_stream->get_event_target());
        m_events->remove_handler(EventType::STREAM_OUTPUT_SHUTDOWN, m_stream->get_event_target());
        m_events->remove_handler(EventType::SOCKET_DISCONNECTED, m_stream->get_event_target());
        m_events->remove_handler(EventType::SOCKET_STOP_RETRY, m_stream->get_event_target());
        cleanupStream();
    }
}

void
Client::cleanupScreen()
{
    if (m_server != nullptr) {
        if (m_ready) {
            m_screen->disable();
            m_ready = false;
        }
        m_events->remove_handler(EventType::SCREEN_SHAPE_CHANGED, get_event_target());
        m_events->remove_handler(EventType::CLIPBOARD_GRABBED, get_event_target());
        delete m_server;
        m_server = nullptr;
    }
}

void
Client::cleanupTimer()
{
    if (m_timer != nullptr) {
        m_events->remove_handler(EventType::TIMER, m_timer);
        m_events->deleteTimer(m_timer);
        m_timer = nullptr;
    }
}

void
Client::cleanupStream()
{
    delete m_stream;
    m_stream = nullptr;
}

void
Client::handle_connected()
{
    LOG_DEBUG1("connected;  wait for hello");
    cleanupConnecting();
    setupConnection();

    // reset clipboard state
    for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        m_ownClipboard[id]  = false;
        m_sentClipboard[id] = false;
        m_timeClipboard[id] = 0;
    }
}

void Client::handle_connection_failed(const Event& event)
{
    const auto& info = event.get_data_as<IDataSocket::ConnectionFailedInfo>();

    cleanupTimer();
    cleanupConnecting();
    cleanupStream();
    LOG_DEBUG1("connection failed");
    sendConnectionFailedEvent(info.m_what.c_str());
}

void Client::handle_connect_timeout()
{
    cleanupTimer();
    cleanupConnecting();
    cleanupConnection();
    cleanupStream();
    LOG_DEBUG1("connection timed out");
    sendConnectionFailedEvent("Timed out");
}

void Client::handle_output_error()
{
    cleanupTimer();
    cleanupScreen();
    cleanupConnection();
    LOG_WARN("error sending to server");
    send_event(EventType::CLIENT_DISCONNECTED);
}

void Client::handle_disconnected()
{
    cleanupTimer();
    cleanupScreen();
    cleanupConnection();
    LOG_DEBUG1("disconnected");
    send_event(EventType::CLIENT_DISCONNECTED);
}

void Client::handle_shape_changed()
{
    LOG_DEBUG("resolution changed");
    m_server->onInfoChanged();
}

void Client::handle_clipboard_grabbed(const Event& event)
{
    if (!m_enableClipboard || (m_maximumClipboardSize == 0)) {
        return;
    }

    const auto& info = event.get_data_as<IScreen::ClipboardInfo>();

    // grab ownership
    m_server->onGrabClipboard(info.m_id);

    // we now own the clipboard and it has not been sent to the server
    m_ownClipboard[info.m_id]  = true;
    m_sentClipboard[info.m_id] = false;
    m_timeClipboard[info.m_id] = 0;

    // if we're not the active screen then send the clipboard now,
    // otherwise we'll wait until we leave.
    if (!m_active) {
        sendClipboard(info.m_id);
    }
}

void Client::handle_hello()
{
    std::int16_t major, minor;
    if (!ProtocolUtil::readf(m_stream, kMsgHello, &major, &minor)) {
        sendConnectionFailedEvent("Protocol error from server, check encryption settings");
        cleanupTimer();
        cleanupConnection();
        return;
    }

    // check versions
    LOG_DEBUG1("got hello version %d.%d", major, minor);
    if (major < kProtocolMajorVersion ||
        (major == kProtocolMajorVersion && minor < kProtocolMinorVersion)) {
        sendConnectionFailedEvent(XIncompatibleClient(major, minor).what());
        cleanupTimer();
        cleanupConnection();
        return;
    }

    // say hello back
    LOG_DEBUG1("say hello version %d.%d", kProtocolMajorVersion, kProtocolMinorVersion);
    ProtocolUtil::writef(m_stream, kMsgHelloBack,
                            kProtocolMajorVersion,
                            kProtocolMinorVersion, &m_name);

    // now connected but waiting to complete handshake
    setupScreen();
    cleanupTimer();

    // make sure we process any remaining messages later.  we won't
    // receive another event for already pending messages so we fake
    // one.
    if (m_stream->isReady()) {
        m_events->add_event(EventType::STREAM_INPUT_READY, m_stream->get_event_target());
    }
}

void Client::handle_suspend()
{
    LOG_INFO("suspend");
    m_suspended       = true;
    bool wasConnected = isConnected();
    disconnect(nullptr);
    m_connectOnResume = wasConnected;
}

void Client::handle_resume()
{
    LOG_INFO("resume");
    m_suspended = false;
    if (m_connectOnResume) {
        m_connectOnResume = false;
        connect();
    }
}

void Client::handle_file_chunk_sending(const Event& event)
{
    send_file_chunk(event.get_data_as<FileChunk>());
}

void Client::handle_file_receive_completed(const Event& event)
{
    (void) event;

    onFileReceiveCompleted();
}

void
Client::onFileReceiveCompleted()
{
    if (isReceivedFileSizeValid()) {
        m_writeToDropDirThread = new Thread([this](){ write_to_drop_dir_thread(); });
    }
}

void Client::handle_stop_retry()
{
    m_args.m_restartable = false;
}

void Client::write_to_drop_dir_thread()
{
    LOG_DEBUG("starting write to drop dir thread");

    while (m_screen->isFakeDraggingStarted()) {
        inputleap::this_thread_sleep(.1f);
    }

    DropHelper::writeToDir(m_screen->getDropTarget(), m_dragFileList,
                    m_receivedFileData);
}

void Client::dragInfoReceived(std::uint32_t fileNum, std::string data)
{
    // TODO: fix duplicate function from CServer
    if (!m_args.m_enableDragDrop) {
        LOG_DEBUG("drag drop not enabled, ignoring drag info.");
        return;
    }

    DragInformation::parseDragInfo(m_dragFileList, fileNum, data);

    m_screen->startDraggingFiles(m_dragFileList);
}

bool
Client::isReceivedFileSizeValid()
{
    return m_expectedFileSize == m_receivedFileData.size();
}

void
Client::sendFileToServer(const char* filename)
{
    if (m_sendFileThread != nullptr) {
        StreamChunker::interruptFile();
    }

    m_sendFileThread = new Thread([this, filename]() { send_file_thread(filename); });
}

void Client::send_file_thread(const char* filename)
{
    try {
        StreamChunker::sendFile(filename, m_events, this);
    }
    catch (std::runtime_error& error) {
        LOG_ERR("failed sending file chunks: %s", error.what());
    }

    m_sendFileThread = nullptr;
}

void Client::sendDragInfo(std::uint32_t fileCount, std::string& info, size_t size)
{
    m_server->sendDragInfo(fileCount, info.c_str(), size);
}

} // namespace inputleap
