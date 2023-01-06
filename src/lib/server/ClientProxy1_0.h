/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#pragma once

#include "server/ClientProxy.h"
#include "inputleap/Clipboard.h"
#include "inputleap/protocol_types.h"

class Event;
class EventQueueTimer;
class IEventQueue;

//! Proxy for client implementing protocol version 1.0
class ClientProxy1_0 : public ClientProxy {
public:
    ClientProxy1_0(const std::string& name, inputleap::IStream* adoptedStream, IEventQueue* events);
    ~ClientProxy1_0() override;

    // IScreen
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
    void sendDragInfo(std::uint32_t fileCount, const char* info, size_t size) override;
    void fileChunkSending(std::uint8_t mark, char* data, size_t dataSize) override;

protected:
    virtual bool parseHandshakeMessage(const std::uint8_t* code);
    virtual bool parseMessage(const std::uint8_t* code);

    virtual void resetHeartbeatRate();
    virtual void setHeartbeatRate(double rate, double alarm);
    virtual void resetHeartbeatTimer();
    virtual void addHeartbeatTimer();
    virtual void removeHeartbeatTimer();
    virtual bool recvClipboard();
private:
    void disconnect();
    void removeHandlers();

    void handleData(const Event&, void*);
    void handleDisconnect(const Event&, void*);
    void handleWriteError(const Event&, void*);
    void handleFlatline(const Event&, void*);

    bool recvInfo();
    bool recvGrabClipboard();

protected:
    struct ClientClipboard {
    public:
        ClientClipboard();

    public:
        Clipboard m_clipboard;
        std::uint32_t m_sequenceNumber;
        bool m_dirty;
    };

    ClientClipboard m_clipboard[kClipboardEnd];

private:
    typedef bool (ClientProxy1_0::*MessageParser)(const std::uint8_t*);

    ClientInfo m_info;
    double m_heartbeatAlarm;
    EventQueueTimer* m_heartbeatTimer;
    MessageParser m_parser;
    IEventQueue* m_events;
};
