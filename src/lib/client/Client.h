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

#pragma once

#include "inputleap/IClient.h"

#include "inputleap/Clipboard.h"
#include "inputleap/DragInformation.h"
#include "inputleap/INode.h"
#include "inputleap/ClientArgs.h"
#include "net/NetworkAddress.h"
#include "base/EventTypes.h"

namespace inputleap {

class EventQueueTimer;
class Screen;
class ServerProxy;
class IDataSocket;
class ISocketFactory;
class IStream;
class IEventQueue;
class Thread;
class TCPSocket;

//! Barrier client
/*!
This class implements the top-level client algorithms for barrier.
*/
class Client : public IClient, public INode {
public:
    class FailInfo {
    public:
        FailInfo(const char* what) : m_retry(false), m_what(what) { }
        bool m_retry;
        std::string m_what;
    };

public:
    /*!
    This client will attempt to connect to the server using \p name
    as its name and \p address as the server's address and \p factory
    to create the socket.  \p screen is    the local screen.
    */
    Client(IEventQueue* events, const std::string& name,
           const NetworkAddress& address, ISocketFactory* socketFactory,
           inputleap::Screen* screen, ClientArgs const& args);

    ~Client();

    //! @name manipulators
    //@{

    //! Connect to server
    /*!
    Starts an attempt to connect to the server.  This is ignored if
    the client is trying to connect or is already connected.
    */
    void connect();

    //! Disconnect
    /*!
    Disconnects from the server with an optional error message.
    */
    void disconnect(const char* msg);

    //! Notify of handshake complete
    /*!
    Notifies the client that the connection handshake has completed.
    */
    virtual void handshakeComplete();

    //! Received drag information
    void dragInfoReceived(std::uint32_t fileNum, std::string data);

    //! Create a new thread and use it to send file to Server
    void sendFileToServer(const char* filename);

    //! Send dragging file information back to server
    void sendDragInfo(std::uint32_t fileCount, std::string& info, size_t size);


    //@}
    //! @name accessors
    //@{

    //! Test if connected
    /*!
    Returns true iff the client is successfully connected to the server.
    */
    bool isConnected() const;

    //! Test if connecting
    /*!
    Returns true iff the client is currently attempting to connect to
    the server.
    */
    bool isConnecting() const;

    //! Get address of server
    /*!
    Returns the address of the server the client is connected (or wants
    to connect) to.
    */
    NetworkAddress getServerAddress() const;

    //! Return true if received file size is valid
    bool isReceivedFileSizeValid();

    //! Return expected file size
    size_t& getExpectedFileSize() { return m_expectedFileSize; }

    //! Return received file data
    std::string& getReceivedFileData() { return m_receivedFileData; }

    //! Return drag file list
    DragFileList getDragFileList() { return m_dragFileList; }

    //@}

    // IScreen overrides
    void* getEventTarget() const override;
    bool getClipboard(ClipboardID id, IClipboard*) const override;
    void getShape(std::int32_t& x, std::int32_t& y, std::int32_t& width,
                  std::int32_t& height) const override;
    void getCursorPos(std::int32_t& x, std::int32_t& y) const override;

    // IClient overrides
    void enter(std::int32_t xAbs, std::int32_t yAbs, std::uint32_t seqNum, KeyModifierMask mask,
               bool forScreensaver) override;
    bool leave() override;
    void setClipboard(ClipboardID, const IClipboard*) override;
    void grabClipboard(ClipboardID) override;
    void setClipboardDirty(ClipboardID, bool) override;
    void keyDown(KeyID, KeyModifierMask, KeyButton) override;
    void keyRepeat(KeyID, KeyModifierMask, std::int32_t count, KeyButton) override;
    void keyUp(KeyID, KeyModifierMask, KeyButton) override;
    void mouseDown(ButtonID) override;
    void mouseUp(ButtonID) override;
    void mouseMove(std::int32_t xAbs, std::int32_t yAbs) override;
    void mouseRelativeMove(std::int32_t xRel, std::int32_t yRel) override;
    void mouseWheel(std::int32_t xDelta, std::int32_t yDelta) override;
    void screensaver(bool activate) override;
    void resetOptions() override;
    void setOptions(const OptionsList& options) override;
    virtual std::string getName() const override;

private:
    void sendClipboard(ClipboardID);
    void sendEvent(EventType, void*);
    void sendConnectionFailedEvent(const char* msg);
    void sendFileChunk(const void* data);
    void send_file_thread(const char* filename);
    void write_to_drop_dir_thread();
    void setupConnecting();
    void setupConnection();
    void setupScreen();
    void setupTimer();
    void cleanupConnecting();
    void cleanupConnection();
    void cleanupScreen();
    void cleanupTimer();
    void cleanupStream();
    void handleConnected(const Event&, void*);
    void handleConnectionFailed(const Event&, void*);
    void handleConnectTimeout(const Event&, void*);
    void handleOutputError(const Event&, void*);
    void handleDisconnected(const Event&, void*);
    void handleShapeChanged(const Event&, void*);
    void handleClipboardGrabbed(const Event&, void*);
    void handleHello(const Event&, void*);
    void handleSuspend(const Event& event, void*);
    void handleResume(const Event& event, void*);
    void handleFileChunkSending(const Event&, void*);
    void handleFileReceiveCompleted(const Event&, void*);
    void handleStopRetry(const Event&, void*);
    void onFileReceiveCompleted();
    void sendClipboardThread(void*);

public:
    bool m_mock;

private:
    std::string m_name;
    NetworkAddress m_serverAddress;
    ISocketFactory* m_socketFactory;
    inputleap::Screen* m_screen;
    inputleap::IStream* m_stream;
    EventQueueTimer* m_timer;
    ServerProxy* m_server;
    bool m_ready;
    bool m_active;
    bool m_suspended;
    bool m_connectOnResume;
    bool m_ownClipboard[kClipboardEnd];
    bool m_sentClipboard[kClipboardEnd];
    IClipboard::Time m_timeClipboard[kClipboardEnd];
    std::string m_dataClipboard[kClipboardEnd];
    IEventQueue* m_events;
    std::size_t m_expectedFileSize;
    std::string m_receivedFileData;
    DragFileList m_dragFileList;
    std::string m_dragFileExt;
    Thread* m_sendFileThread;
    Thread* m_writeToDropDirThread;
    TCPSocket* m_socket;
    bool m_useSecureNetwork;
    ClientArgs m_args;
    bool m_enableClipboard;
};

} // namespace inputleap
