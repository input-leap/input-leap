/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "server/ClientProxyUnknown.h"
#include "ClientConnectionByStream.h"
#include "ClientConnectionLoggingWrapper.h"
#include "base/ELevel.h"
#include "server/Server.h"
#include "server/ClientProxy1_6.h"
#include "inputleap/protocol_types.h"
#include "inputleap/ProtocolUtil.h"
#include "inputleap/Exceptions.h"
#include "io/IStream.h"
#include "io/XIO.h"
#include "base/Log.h"
#include "base/IEventQueue.h"

namespace inputleap {

ClientProxyUnknown::ClientProxyUnknown(std::unique_ptr<inputleap::IStream> stream,
                                       double timeout, Server* server, IEventQueue* events) :
    stream_(std::move(stream)),
    m_proxy(nullptr),
    m_ready(false),
    m_server(server),
    m_events(events)
{
    assert(m_server != nullptr);
    m_events->add_handler(EventType::TIMER, this,
                          [this](const auto& e){ handle_timeout(); });
    m_timer = m_events->newOneShotTimer(timeout, this);
    addStreamHandlers();

    LOG_DEBUG1("saying hello");
    ProtocolUtil::writef(stream_.get(), kMsgHello, kProtocolMajorVersion, kProtocolMinorVersion);
}

ClientProxyUnknown::~ClientProxyUnknown()
{
    remove_handlers();
    removeTimer();
    delete m_proxy;
}

ClientProxy*
ClientProxyUnknown::orphanClientProxy()
{
    if (m_ready) {
        remove_handlers();
        ClientProxy* proxy = m_proxy;
        m_proxy = nullptr;
        return proxy;
    }
    else {
        return nullptr;
    }
}

void
ClientProxyUnknown::sendSuccess()
{
    m_ready = true;
    removeTimer();
    m_events->add_event(EventType::CLIENT_PROXY_UNKNOWN_SUCCESS, this);
}

void
ClientProxyUnknown::sendFailure()
{
    delete m_proxy;
    m_proxy = nullptr;
    m_ready = false;
    remove_handlers();
    removeTimer();
    m_events->add_event(EventType::CLIENT_PROXY_UNKNOWN_FAILURE, this);
}

void
ClientProxyUnknown::addStreamHandlers()
{
    assert(stream_.get() != nullptr);
    m_events->add_handler(EventType::STREAM_INPUT_READY, stream_->get_event_target(),
                          [this](const auto& e){ handle_data(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_ERROR, stream_->get_event_target(),
                          [this](const auto& e){ handle_write_error(); });
    m_events->add_handler(EventType::STREAM_INPUT_SHUTDOWN, stream_->get_event_target(),
                          [this](const auto& e){ handle_disconnect(); });
    m_events->add_handler(EventType::STREAM_INPUT_FORMAT_ERROR, stream_->get_event_target(),
                          [this](const auto& e){ handle_disconnect(); });
    m_events->add_handler(EventType::STREAM_OUTPUT_SHUTDOWN, stream_->get_event_target(),
                          [this](const auto& e){ handle_write_error(); });
}

void
ClientProxyUnknown::addProxyHandlers()
{
    assert(m_proxy != nullptr);
    m_events->add_handler(EventType::CLIENT_PROXY_READY, m_proxy,
                          [this](const auto& e){ handle_ready(); });
    m_events->add_handler(EventType::CLIENT_PROXY_DISCONNECTED, m_proxy,
                          [this](const auto& e){ handle_disconnect(); });
}

void
ClientProxyUnknown::remove_handlers()
{
    if (stream_) {
        m_events->remove_handler(EventType::STREAM_INPUT_READY, stream_->get_event_target());
        m_events->remove_handler(EventType::STREAM_OUTPUT_ERROR, stream_->get_event_target());
        m_events->remove_handler(EventType::STREAM_INPUT_SHUTDOWN, stream_->get_event_target());
        m_events->remove_handler(EventType::STREAM_INPUT_FORMAT_ERROR, stream_->get_event_target());
        m_events->remove_handler(EventType::STREAM_OUTPUT_SHUTDOWN, stream_->get_event_target());
    }
    if (m_proxy != nullptr) {
        m_events->remove_handler(EventType::CLIENT_PROXY_READY, m_proxy);
        m_events->remove_handler(EventType::CLIENT_PROXY_DISCONNECTED, m_proxy);
    }
}

void
ClientProxyUnknown::removeTimer()
{
    if (m_timer != nullptr) {
        m_events->deleteTimer(m_timer);
        m_events->remove_handler(EventType::TIMER, this);
        m_timer = nullptr;
    }
}

void ClientProxyUnknown::handle_data()
{
    LOG_DEBUG1("parsing hello reply");

    std::string name("<unknown>");
    try {
        // limit the maximum length of the hello
        std::uint32_t n = stream_->getSize();
        if (n > kMaxHelloLength) {
            LOG_DEBUG1("hello reply too long");
            throw XBadClient();
        }

        // parse the reply to hello
        std::int16_t major, minor;
        if (!ProtocolUtil::readf(stream_.get(), kMsgHelloBack, &major, &minor, &name)) {
            throw XBadClient();
        }

        // disallow invalid version numbers
        if (major <= 0 || minor < 0) {
            throw XIncompatibleClient(major, minor);
        }

        // remove stream event handlers.  the proxy we're about to create
        // may install its own handlers and we don't want to accidentally
        // remove those later.
        remove_handlers();

        {
            std::unique_ptr<IClientConnection> conn =
                    std::make_unique<ClientConnectionByStream>(std::move(stream_));

            if (Log::getInstance()->getFilter() >= kDEBUG1) {
                conn = std::make_unique<ClientConnectionLoggingWrapper>(name, std::move(conn));
            }

            // create client proxy for highest version supported by the client
            if (major == 1) {
                switch (minor) {
                case 6:
                    m_proxy = new ClientProxy1_6(name, std::move(conn), m_server, m_events);
                    break;
                default:
                    break;
                }
            }
        }

        // hangup (with error) if version isn't supported
        if (m_proxy == nullptr) {
            throw XIncompatibleClient(major, minor);
        }

        LOG_DEBUG1("created proxy for client \"%s\" version %d.%d", name.c_str(), major, minor);

        // wait until the proxy signals that it's ready or has disconnected
        addProxyHandlers();
        return;
    }
    catch (XIncompatibleClient& e) {
        // client is incompatible
        LOG_WARN("client \"%s\" has incompatible version %d.%d)", name.c_str(), e.getMajor(), e.getMinor());
        ProtocolUtil::writef(stream_.get(), kMsgEIncompatible,
                             kProtocolMajorVersion, kProtocolMinorVersion);
    }
    catch (XBadClient&) {
        // client not behaving
        LOG_WARN("protocol error from client \"%s\"", name.c_str());
        ProtocolUtil::writef(stream_.get(), kMsgEBad);
    }
    catch (XBase& e) {
        // misc error
        LOG_WARN("error communicating with client \"%s\": %s", name.c_str(), e.what());
    }
    sendFailure();
}

void ClientProxyUnknown::handle_write_error()
{
    LOG_NOTE("error communicating with new client");
    sendFailure();
}

void ClientProxyUnknown::handle_timeout()
{
    LOG_NOTE("new client is unresponsive");
    sendFailure();
}

void ClientProxyUnknown::handle_disconnect()
{
    LOG_NOTE("new client disconnected");
    sendFailure();
}

void ClientProxyUnknown::handle_ready()
{
    sendSuccess();
}

} // namespace inputleap
