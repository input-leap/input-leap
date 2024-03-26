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

#include "client/ServerProxy.h"

#include "client/Client.h"
#include "inputleap/FileChunk.h"
#include "inputleap/ClipboardChunk.h"
#include "inputleap/StreamChunker.h"
#include "inputleap/Clipboard.h"
#include "inputleap/ProtocolUtil.h"
#include "inputleap/option_types.h"
#include "inputleap/protocol_types.h"
#include "inputleap/Exceptions.h"
#include "io/IStream.h"
#include "base/Log.h"
#include "base/IEventQueue.h"
#include "base/EventQueueTimer.h"
#include "base/XBase.h"

#include <memory>

namespace inputleap {

ServerProxy::ServerProxy(Client* client, inputleap::IStream* stream, IEventQueue* events) :
    m_client(client),
    m_stream(stream),
    m_seqNum(0),
    m_compressMouse(false),
    m_compressMouseRelative(false),
    m_xMouse(0),
    m_yMouse(0),
    m_dxMouse(0),
    m_dyMouse(0),
    m_ignoreMouse(false),
    m_keepAliveAlarm(0.0),
    m_keepAliveAlarmTimer(nullptr),
    m_parser(&ServerProxy::parseHandshakeMessage),
    m_events(events)
{
    assert(m_client != nullptr);
    assert(m_stream != nullptr);

    // initialize modifier translation table
    for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id)
        m_modifierTranslationTable[id] = id;

    // handle data on stream
    m_events->add_handler(EventType::STREAM_INPUT_READY, m_stream->get_event_target(),
                          [this](const auto& e){ handle_data(); });
    m_events->add_handler(EventType::CLIPBOARD_SENDING, this,
                          [this](const auto& e){ handle_clipboard_sending_event(e); });

    // send heartbeat
    setKeepAliveRate(kKeepAliveRate);
}

ServerProxy::~ServerProxy()
{
    setKeepAliveRate(-1.0);
    m_events->remove_handler(EventType::STREAM_INPUT_READY, m_stream->get_event_target());
    m_events->remove_handler(EventType::CLIPBOARD_SENDING, this);
}

void
ServerProxy::resetKeepAliveAlarm()
{
    if (m_keepAliveAlarmTimer != nullptr) {
        m_events->remove_handler(EventType::TIMER, m_keepAliveAlarmTimer);
        m_events->deleteTimer(m_keepAliveAlarmTimer);
        m_keepAliveAlarmTimer = nullptr;
    }
    if (m_keepAliveAlarm > 0.0) {
        m_keepAliveAlarmTimer =
            m_events->newOneShotTimer(m_keepAliveAlarm, nullptr);
        m_events->add_handler(EventType::TIMER, m_keepAliveAlarmTimer,
                              [this](const auto& e){ handle_keep_alive_alarm(); });
    }
}

void
ServerProxy::setKeepAliveRate(double rate)
{
    m_keepAliveAlarm = rate * kKeepAlivesUntilDeath;
    resetKeepAliveAlarm();
}

void ServerProxy::handle_data()
{
    // handle messages until there are no more.  first read message code.
    std::uint8_t code[4];
    std::uint32_t n = m_stream->read(code, 4);
    while (n != 0) {
        // verify we got an entire code
        if (n != 4) {
            LOG_ERR("incomplete message from server: %d bytes", n);
            m_client->disconnect("incomplete message from server");
            return;
        }

        // parse message
        LOG_DEBUG2("msg from server: %c%c%c%c", code[0], code[1], code[2], code[3]);
        try {
            switch ((this->*m_parser)(code)) {
            case kOkay:
                break;

            case kUnknown:
                LOG_ERR("invalid message from server: %c%c%c%c", code[0], code[1], code[2], code[3]);
                m_client->disconnect("invalid message from server");
                return;

            case kDisconnect:
                return;
            default:
                break;
            }
        } catch (const XBadClient& e) {
            // TODO: disconnect handling is currently dispersed across both parseMessage() and
            // handleData() functions, we should collect that to a single place

            LOG_ERR("protocol error from server: %s", e.what());
            ProtocolUtil::writef(m_stream, kMsgEBad);
            m_client->disconnect("invalid message from server");
            return;
        }

        // next message
        n = m_stream->read(code, 4);
    }

    flushCompressedMouse();
}

ServerProxy::EResult ServerProxy::parseHandshakeMessage(const std::uint8_t* code)
{
    if (memcmp(code, kMsgQInfo, 4) == 0) {
        queryInfo();
    }

    else if (memcmp(code, kMsgCInfoAck, 4) == 0) {
        infoAcknowledgment();
    }

    else if (memcmp(code, kMsgDSetOptions, 4) == 0) {
        setOptions();

        // handshake is complete
        m_parser = &ServerProxy::parseMessage;
        m_client->handshakeComplete();
    }

    else if (memcmp(code, kMsgCResetOptions, 4) == 0) {
        resetOptions();
    }

    else if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
        // echo keep alives and reset alarm
        ProtocolUtil::writef(m_stream, kMsgCKeepAlive);
        resetKeepAliveAlarm();
    }

    else if (memcmp(code, kMsgCNoop, 4) == 0) {
        // accept and discard no-op
    }

    else if (memcmp(code, kMsgCClose, 4) == 0) {
        // server wants us to hangup
        LOG_DEBUG1("recv close");
        m_client->disconnect(nullptr);
        return kDisconnect;
    }

    else if (memcmp(code, kMsgEIncompatible, 4) == 0) {
        std::int32_t major, minor;
        ProtocolUtil::readf(m_stream,
                        kMsgEIncompatible + 4, &major, &minor);
        LOG_ERR("server has incompatible version %d.%d", major, minor);
        m_client->disconnect("server has incompatible version");
        return kDisconnect;
    }

    else if (memcmp(code, kMsgEBusy, 4) == 0) {
        LOG_ERR("server already has a connected client with name \"%s\"", m_client->getName().c_str());
        m_client->disconnect("server already has a connected client with our name");
        return kDisconnect;
    }

    else if (memcmp(code, kMsgEUnknown, 4) == 0) {
        LOG_ERR("server refused client with name \"%s\"", m_client->getName().c_str());
        m_client->disconnect("server refused client with our name");
        return kDisconnect;
    }

    else if (memcmp(code, kMsgEBad, 4) == 0) {
        LOG_ERR("server disconnected due to a protocol error");
        m_client->disconnect("server reported a protocol error");
        return kDisconnect;
    }
    else {
        return kUnknown;
    }

    return kOkay;
}

ServerProxy::EResult ServerProxy::parseMessage(const std::uint8_t* code)
{
    if (memcmp(code, kMsgDMouseMove, 4) == 0) {
        mouseMove();
    }

    else if (memcmp(code, kMsgDMouseRelMove, 4) == 0) {
        mouseRelativeMove();
    }

    else if (memcmp(code, kMsgDMouseWheel, 4) == 0) {
        mouseWheel();
    }

    else if (memcmp(code, kMsgDKeyDown, 4) == 0) {
        keyDown();
    }

    else if (memcmp(code, kMsgDKeyUp, 4) == 0) {
        keyUp();
    }

    else if (memcmp(code, kMsgDMouseDown, 4) == 0) {
        mouseDown();
    }

    else if (memcmp(code, kMsgDMouseUp, 4) == 0) {
        mouseUp();
    }

    else if (memcmp(code, kMsgDKeyRepeat, 4) == 0) {
        keyRepeat();
    }

    else if (memcmp(code, kMsgCKeepAlive, 4) == 0) {
        // echo keep alives and reset alarm
        ProtocolUtil::writef(m_stream, kMsgCKeepAlive);
        resetKeepAliveAlarm();
    }

    else if (memcmp(code, kMsgCNoop, 4) == 0) {
        // accept and discard no-op
    }

    else if (memcmp(code, kMsgCEnter, 4) == 0) {
        enter();
    }

    else if (memcmp(code, kMsgCLeave, 4) == 0) {
        leave();
    }

    else if (memcmp(code, kMsgCClipboard, 4) == 0) {
        grabClipboard();
    }

    else if (memcmp(code, kMsgCScreenSaver, 4) == 0) {
        screensaver();
    }

    else if (memcmp(code, kMsgQInfo, 4) == 0) {
        queryInfo();
    }

    else if (memcmp(code, kMsgCInfoAck, 4) == 0) {
        infoAcknowledgment();
    }

    else if (memcmp(code, kMsgDClipboard, 4) == 0) {
        setClipboard();
    }

    else if (memcmp(code, kMsgCResetOptions, 4) == 0) {
        resetOptions();
    }

    else if (memcmp(code, kMsgDSetOptions, 4) == 0) {
        setOptions();
    }

    else if (memcmp(code, kMsgDFileTransfer, 4) == 0) {
        fileChunkReceived();
    }
    else if (memcmp(code, kMsgDDragInfo, 4) == 0) {
        dragInfoReceived();
    }

    else if (memcmp(code, kMsgCClose, 4) == 0) {
        // server wants us to hangup
        LOG_DEBUG1("recv close");
        m_client->disconnect(nullptr);
        return kDisconnect;
    }
    else if (memcmp(code, kMsgEBad, 4) == 0) {
        LOG_ERR("server disconnected due to a protocol error");
        m_client->disconnect("server reported a protocol error");
        return kDisconnect;
    }
    else {
        return kUnknown;
    }

    // send a reply.  this is intended to work around a delay when
    // running a linux server and an OS X (any BSD?) client.  the
    // client waits to send an ACK (if the system control flag
    // net.inet.tcp.delayed_ack is 1) in hopes of piggybacking it
    // on a data packet.  we provide that packet here.  i don't
    // know why a delayed ACK should cause the server to wait since
    // TCP_NODELAY is enabled.
    ProtocolUtil::writef(m_stream, kMsgCNoop);

    return kOkay;
}

void ServerProxy::handle_keep_alive_alarm()
{
    LOG_NOTE("server is dead");
    m_client->disconnect("server is not responding");
}

void
ServerProxy::onInfoChanged()
{
    // ignore mouse motion until we receive acknowledgment of our info
    // change message.
    m_ignoreMouse = true;

    // send info update
    queryInfo();
}

bool
ServerProxy::onGrabClipboard(ClipboardID id)
{
    LOG_DEBUG1("sending clipboard %d changed", id);
    ProtocolUtil::writef(m_stream, kMsgCClipboard, id, m_seqNum);
    return true;
}

void
ServerProxy::onClipboardChanged(ClipboardID id, const IClipboard* clipboard)
{
    std::string data = IClipboard::marshall(clipboard);
    LOG_DEBUG("sending clipboard %d seqnum=%d", id, m_seqNum);

    StreamChunker::sendClipboard(data, data.size(), id, m_seqNum, m_events, this);
}

void
ServerProxy::flushCompressedMouse()
{
    if (m_compressMouse) {
        m_compressMouse = false;
        m_client->mouseMove(m_xMouse, m_yMouse);
    }
    if (m_compressMouseRelative) {
        m_compressMouseRelative = false;
        m_client->mouseRelativeMove(m_dxMouse, m_dyMouse);
        m_dxMouse = 0;
        m_dyMouse = 0;
    }
}

void
ServerProxy::sendInfo(const ClientInfo& info)
{
    LOG_DEBUG1("sending info shape=%d,%d %dx%d", info.m_x, info.m_y, info.m_w, info.m_h);
    ProtocolUtil::writef(m_stream, kMsgDInfo,
                                info.m_x, info.m_y,
                                info.m_w, info.m_h, 0,
                                info.m_mx, info.m_my);
}

KeyID
ServerProxy::translateKey(KeyID id) const
{
    static const KeyID s_translationTable[kKeyModifierIDLast][2] = {
        { kKeyNone,      kKeyNone },
        { kKeyShift_L,   kKeyShift_R },
        { kKeyControl_L, kKeyControl_R },
        { kKeyAlt_L,     kKeyAlt_R },
        { kKeyMeta_L,    kKeyMeta_R },
        { kKeySuper_L,   kKeySuper_R },
        { kKeyAltGr,     kKeyAltGr}
    };

    KeyModifierID id2 = kKeyModifierIDNull;
    std::uint32_t side = 0;
    switch (id) {
    case kKeyShift_L:
        id2  = kKeyModifierIDShift;
        side = 0;
        break;

    case kKeyShift_R:
        id2  = kKeyModifierIDShift;
        side = 1;
        break;

    case kKeyControl_L:
        id2  = kKeyModifierIDControl;
        side = 0;
        break;

    case kKeyControl_R:
        id2  = kKeyModifierIDControl;
        side = 1;
        break;

    case kKeyAlt_L:
        id2  = kKeyModifierIDAlt;
        side = 0;
        break;

    case kKeyAlt_R:
        id2  = kKeyModifierIDAlt;
        side = 1;
        break;

    case kKeyAltGr:
        id2 = kKeyModifierIDAltGr;
        side = 1; // there is only one alt gr key on the right side
        break;

    case kKeyMeta_L:
        id2  = kKeyModifierIDMeta;
        side = 0;
        break;

    case kKeyMeta_R:
        id2  = kKeyModifierIDMeta;
        side = 1;
        break;

    case kKeySuper_L:
        id2  = kKeyModifierIDSuper;
        side = 0;
        break;

    case kKeySuper_R:
        id2  = kKeyModifierIDSuper;
        side = 1;
        break;
    default:
        break;
    }

    if (id2 != kKeyModifierIDNull) {
        return s_translationTable[m_modifierTranslationTable[id2]][side];
    }
    else {
        return id;
    }
}

KeyModifierMask
ServerProxy::translateModifierMask(KeyModifierMask mask) const
{
    static const KeyModifierMask s_masks[kKeyModifierIDLast] = {
        0x0000,
        KeyModifierShift,
        KeyModifierControl,
        KeyModifierAlt,
        KeyModifierMeta,
        KeyModifierSuper,
        KeyModifierAltGr
    };

    KeyModifierMask newMask = mask & ~(KeyModifierShift |
                                        KeyModifierControl |
                                        KeyModifierAlt |
                                        KeyModifierMeta |
                                        KeyModifierSuper |
                                        KeyModifierAltGr );
    if ((mask & KeyModifierShift) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDShift]];
    }
    if ((mask & KeyModifierControl) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDControl]];
    }
    if ((mask & KeyModifierAlt) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDAlt]];
    }
    if ((mask & KeyModifierAltGr) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDAltGr]];
    }
    if ((mask & KeyModifierMeta) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDMeta]];
    }
    if ((mask & KeyModifierSuper) != 0) {
        newMask |= s_masks[m_modifierTranslationTable[kKeyModifierIDSuper]];
    }
    return newMask;
}

void
ServerProxy::enter()
{
    // parse
    std::int16_t x, y;
    std::uint16_t mask;
    std::uint32_t seqNum;
    ProtocolUtil::readf(m_stream, kMsgCEnter + 4, &x, &y, &seqNum, &mask);
    LOG_DEBUG1("recv enter, %d,%d %d %04x", x, y, seqNum, mask);

    // discard old compressed mouse motion, if any
    m_compressMouse         = false;
    m_compressMouseRelative = false;
    m_dxMouse               = 0;
    m_dyMouse               = 0;
    m_seqNum                = seqNum;

    // forward
    m_client->enter(x, y, seqNum, static_cast<KeyModifierMask>(mask), false);
}

void
ServerProxy::leave()
{
    // parse
    LOG_DEBUG1("recv leave");

    // send last mouse motion
    flushCompressedMouse();

    // forward
    m_client->leave();
}

void
ServerProxy::setClipboard()
{
    // parse
    static std::string dataCached;
    ClipboardID id;
    std::uint32_t seq;

    int r = ClipboardChunk::assemble(m_stream, dataCached, id, seq);

    if (r == kStart) {
        size_t size = ClipboardChunk::getExpectedSize();
        LOG_DEBUG("receiving clipboard %d size=%zd", id, size);
    }
    else if (r == kFinish) {
        LOG_DEBUG("received clipboard %d size=%zd", id, dataCached.size());

        // forward
        Clipboard clipboard;
        clipboard.unmarshall(dataCached, 0);
        m_client->setClipboard(id, &clipboard);

        LOG_INFO("clipboard was updated");
    }
}

void
ServerProxy::grabClipboard()
{
    // parse
    ClipboardID id;
    std::uint32_t seqNum;
    ProtocolUtil::readf(m_stream, kMsgCClipboard + 4, &id, &seqNum);
    LOG_DEBUG("recv grab clipboard %d", id);

    // validate
    if (id >= kClipboardEnd) {
        return;
    }

    // forward
    m_client->grabClipboard(id);
}

void
ServerProxy::keyDown()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::uint16_t id, mask, button;
    ProtocolUtil::readf(m_stream, kMsgDKeyDown + 4, &id, &mask, &button);
    LOG_DEBUG1("recv key down id=0x%08x, mask=0x%04x, button=0x%04x", id, mask, button);

    // translate
    KeyID id2             = translateKey(static_cast<KeyID>(id));
    KeyModifierMask mask2 = translateModifierMask(
                                static_cast<KeyModifierMask>(mask));
    if (id2   != static_cast<KeyID>(id) ||
        mask2 != static_cast<KeyModifierMask>(mask))
        LOG_DEBUG1("key down translated to id=0x%08x, mask=0x%04x", id2, mask2);

    // forward
    m_client->keyDown(id2, mask2, button);
}

void
ServerProxy::keyRepeat()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::uint16_t id, mask, count, button;
    ProtocolUtil::readf(m_stream, kMsgDKeyRepeat + 4,
                                &id, &mask, &count, &button);
    LOG_DEBUG1("recv key repeat id=0x%08x, mask=0x%04x, count=%d, button=0x%04x", id, mask, count, button);

    // translate
    KeyID id2             = translateKey(static_cast<KeyID>(id));
    KeyModifierMask mask2 = translateModifierMask(
                                static_cast<KeyModifierMask>(mask));
    if (id2   != static_cast<KeyID>(id) ||
        mask2 != static_cast<KeyModifierMask>(mask))
        LOG_DEBUG1("key repeat translated to id=0x%08x, mask=0x%04x", id2, mask2);

    // forward
    m_client->keyRepeat(id2, mask2, count, button);
}

void
ServerProxy::keyUp()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::uint16_t id, mask, button;
    ProtocolUtil::readf(m_stream, kMsgDKeyUp + 4, &id, &mask, &button);
    LOG_DEBUG1("recv key up id=0x%08x, mask=0x%04x, button=0x%04x", id, mask, button);

    // translate
    KeyID id2             = translateKey(static_cast<KeyID>(id));
    KeyModifierMask mask2 = translateModifierMask(
                                static_cast<KeyModifierMask>(mask));
    if (id2   != static_cast<KeyID>(id) ||
        mask2 != static_cast<KeyModifierMask>(mask))
        LOG_DEBUG1("key up translated to id=0x%08x, mask=0x%04x", id2, mask2);

    // forward
    m_client->keyUp(id2, mask2, button);
}

void
ServerProxy::mouseDown()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::int8_t id;
    ProtocolUtil::readf(m_stream, kMsgDMouseDown + 4, &id);
    LOG_DEBUG1("recv mouse down id=%d", id);

    // forward
    m_client->mouseDown(static_cast<ButtonID>(id));
}

void
ServerProxy::mouseUp()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::int8_t id;
    ProtocolUtil::readf(m_stream, kMsgDMouseUp + 4, &id);
    LOG_DEBUG1("recv mouse up id=%d", id);

    // forward
    m_client->mouseUp(static_cast<ButtonID>(id));
}

void
ServerProxy::mouseMove()
{
    // parse
    bool ignore;
    std::int16_t x, y;
    ProtocolUtil::readf(m_stream, kMsgDMouseMove + 4, &x, &y);

    // note if we should ignore the move
    ignore = m_ignoreMouse;

    // compress mouse motion events if more input follows
    if (!ignore && !m_compressMouse && m_stream->isReady()) {
        m_compressMouse = true;
    }

    // if compressing then ignore the motion but record it
    if (m_compressMouse) {
        m_compressMouseRelative = false;
        ignore    = true;
        m_xMouse  = x;
        m_yMouse  = y;
        m_dxMouse = 0;
        m_dyMouse = 0;
    }
    LOG_DEBUG2("recv mouse move %d,%d", x, y);

    // forward
    if (!ignore) {
        m_client->mouseMove(x, y);
    }
}

void
ServerProxy::mouseRelativeMove()
{
    // parse
    bool ignore;
    std::int16_t dx, dy;
    ProtocolUtil::readf(m_stream, kMsgDMouseRelMove + 4, &dx, &dy);

    // note if we should ignore the move
    ignore = m_ignoreMouse;

    // compress mouse motion events if more input follows
    if (!ignore && !m_compressMouseRelative && m_stream->isReady()) {
        m_compressMouseRelative = true;
    }

    // if compressing then ignore the motion but record it
    if (m_compressMouseRelative) {
        ignore     = true;
        m_dxMouse += dx;
        m_dyMouse += dy;
    }
    LOG_DEBUG2("recv mouse relative move %d,%d", dx, dy);

    // forward
    if (!ignore) {
        m_client->mouseRelativeMove(dx, dy);
    }
}

void
ServerProxy::mouseWheel()
{
    // get mouse up to date
    flushCompressedMouse();

    // parse
    std::int16_t xDelta, yDelta;
    ProtocolUtil::readf(m_stream, kMsgDMouseWheel + 4, &xDelta, &yDelta);
    LOG_DEBUG2("recv mouse wheel %+d,%+d", xDelta, yDelta);

    // forward
    m_client->mouseWheel(xDelta, yDelta);
}

void
ServerProxy::screensaver()
{
    // parse
    std::int8_t on;
    ProtocolUtil::readf(m_stream, kMsgCScreenSaver + 4, &on);
    LOG_DEBUG1("recv screen saver on=%d", on);

    // forward
    m_client->screensaver(on != 0);
}

void
ServerProxy::resetOptions()
{
    // parse
    LOG_DEBUG1("recv reset options");

    // forward
    m_client->resetOptions();

    // reset keep alive
    setKeepAliveRate(kKeepAliveRate);

    // reset modifier translation table
    for (KeyModifierID id = 0; id < kKeyModifierIDLast; ++id) {
        m_modifierTranslationTable[id] = id;
    }
}

void
ServerProxy::setOptions()
{
    // parse
    OptionsList options;
    ProtocolUtil::readf(m_stream, kMsgDSetOptions + 4, &options);
    LOG_DEBUG1("recv set options size=%zd", options.size());

    // forward
    m_client->setOptions(options);

    // update modifier table
    for (std::uint32_t i = 0, n = static_cast<std::uint32_t>(options.size()); i < n; i += 2) {
        KeyModifierID id = kKeyModifierIDNull;
        if (options[i] == kOptionModifierMapForShift) {
            id = kKeyModifierIDShift;
        }
        else if (options[i] == kOptionModifierMapForControl) {
            id = kKeyModifierIDControl;
        }
        else if (options[i] == kOptionModifierMapForAlt) {
            id = kKeyModifierIDAlt;
        }
        else if (options[i] == kOptionModifierMapForAltGr) {
            id = kKeyModifierIDAltGr;
        }
        else if (options[i] == kOptionModifierMapForMeta) {
            id = kKeyModifierIDMeta;
        }
        else if (options[i] == kOptionModifierMapForSuper) {
            id = kKeyModifierIDSuper;
        }
        else if (options[i] == kOptionHeartbeat) {
            // update keep alive
            setKeepAliveRate(1.0e-3 * static_cast<double>(options[i + 1]));
        }

        if (id != kKeyModifierIDNull) {
            m_modifierTranslationTable[id] =
                static_cast<KeyModifierID>(options[i + 1]);
            LOG_DEBUG1("modifier %d mapped to %d", id, m_modifierTranslationTable[id]);
        }
    }
}

void
ServerProxy::queryInfo()
{
    ClientInfo info;
    m_client->getShape(info.m_x, info.m_y, info.m_w, info.m_h);
    m_client->getCursorPos(info.m_mx, info.m_my);
    sendInfo(info);
}

void
ServerProxy::infoAcknowledgment()
{
    LOG_DEBUG1("recv info acknowledgment");
    m_ignoreMouse = false;
}

void
ServerProxy::fileChunkReceived()
{
    int result = FileChunk::assemble(
                    m_stream,
                    m_client->getReceivedFileData(),
                    m_client->getExpectedFileSize());

    if (result == kFinish) {
        m_events->add_event(EventType::FILE_RECEIVE_COMPLETED, m_client);
    }
    else if (result == kStart) {
        if (m_client->getDragFileList().size() > 0) {
            std::string filename = m_client->getDragFileList().at(0).getFilename();
            LOG_DEBUG("start receiving %s", filename.c_str());
        }
    }
}

void
ServerProxy::dragInfoReceived()
{
    // parse
    std::uint32_t fileNum = 0;
    std::string content;
    ProtocolUtil::readf(m_stream, kMsgDDragInfo + 4, &fileNum, &content);

    m_client->dragInfoReceived(fileNum, content);
}

void ServerProxy::handle_clipboard_sending_event(const Event& event)
{
    const auto& chunk = event.get_data_as<ClipboardChunk>();
    ProtocolUtil::writef(m_stream, kMsgDClipboard, chunk.id_, chunk.sequence_, chunk.mark_,
                         &chunk.data_);
}

void ServerProxy::file_chunk_sending(const FileChunk& chunk)
{
    ProtocolUtil::writef(m_stream, kMsgDFileTransfer, chunk.mark_, &chunk.data_);
}

void ServerProxy::sendDragInfo(std::uint32_t fileCount, const char* info, size_t size)
{
    std::string data(info, size);
    ProtocolUtil::writef(m_stream, kMsgDDragInfo, fileCount, &data);
}

} // namespace inputleap
