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

#include "server/Server.h"

#include "server/ClientProxy.h"
#include "server/ClientProxyUnknown.h"
#include "server/PrimaryClient.h"
#include "server/ClientListener.h"
#include "inputleap/FileChunk.h"
#include "inputleap/IPlatformScreen.h"
#include "inputleap/DropHelper.h"
#include "inputleap/option_types.h"
#include "inputleap/protocol_types.h"
#include "inputleap/XScreen.h"
#include "inputleap/Exceptions.h"
#include "inputleap/StreamChunker.h"
#include "inputleap/KeyState.h"
#include "inputleap/Screen.h"
#include "inputleap/PacketStreamFilter.h"
#include "net/TCPSocket.h"
#include "net/IDataSocket.h"
#include "net/IListenSocket.h"
#include "net/XSocket.h"
#include "mt/Thread.h"
#include "arch/Arch.h"
#include "base/IEventQueue.h"
#include "base/Log.h"
#include "base/Time.h"

#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <ctime>
#include <stdexcept>

namespace inputleap {

Server::Server(
		Config& config,
		PrimaryClient* primaryClient,
		inputleap::Screen* screen,
		IEventQueue* events,
		ServerArgs const& args) :
	m_mock(false),
	m_primaryClient(primaryClient),
	m_active(primaryClient),
	m_seqNum(0),
	m_xDelta(0),
	m_yDelta(0),
	m_xDelta2(0),
	m_yDelta2(0),
	m_config(&config),
	m_inputFilter(config.getInputFilter()),
	m_activeSaver(nullptr),
	m_switchDir(kNoDirection),
	m_switchScreen(nullptr),
	m_switchWaitDelay(0.0),
	m_switchWaitTimer(nullptr),
	m_switchTwoTapDelay(0.0),
	m_switchTwoTapEngaged(false),
	m_switchTwoTapArmed(false),
	m_switchTwoTapZone(3),
	m_switchNeedsShift(false),
	m_switchNeedsControl(false),
	m_switchNeedsAlt(false),
	m_relativeMoves(false),
	m_keyboardBroadcasting(false),
	m_lockedToScreen(false),
	m_screen(screen),
	m_events(events),
	m_sendFileThread(nullptr),
	m_writeToDropDirThread(nullptr),
	m_ignoreFileTransfer(false),
	m_enableClipboard(true),
	m_sendDragInfoThread(nullptr),
	m_waitDragInfoThread(true),
	m_args(args)
{
	// must have a primary client and it must have a canonical name
	assert(m_primaryClient != nullptr);
	assert(config.isScreen(primaryClient->getName()));
	assert(m_screen != nullptr);

    std::string primaryName = getName(primaryClient);

	// clear clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		ClipboardInfo& clipboard   = m_clipboards[id];
		clipboard.m_clipboardOwner  = primaryName;
		clipboard.m_clipboardSeqNum = m_seqNum;
		if (clipboard.m_clipboard.open(0)) {
			clipboard.m_clipboard.empty();
			clipboard.m_clipboard.close();
		}
		clipboard.m_clipboardData   = clipboard.m_clipboard.marshall();
	}

    // install event handlers
    m_events->add_handler(EventType::TIMER, this,
                          [this](const auto& e){ handle_switch_wait_event(); });
    m_events->add_handler(EventType::KEY_STATE_KEY_DOWN, m_inputFilter,
                          [this](const auto& e){ handle_key_down_event(e); });
    m_events->add_handler(EventType::KEY_STATE_KEY_UP, m_inputFilter,
                          [this](const auto& e){ handle_key_up_event(e); });
    m_events->add_handler(EventType::KEY_STATE_KEY_REPEAT, m_inputFilter,
                          [this](const auto& e){ handle_key_repeat_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_BUTTON_DOWN, m_inputFilter,
                          [this](const auto& e){ handle_button_down_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_BUTTON_UP, m_inputFilter,
                          [this](const auto& e){ handle_button_up_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY,
                          m_primaryClient->getEventTarget(),
                          [this](const auto& e){ handle_motion_primary_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY,
                          m_primaryClient->getEventTarget(),
                          [this](const auto& e){ handle_motion_secondary_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_WHEEL,
                          m_primaryClient->getEventTarget(),
                          [this](const auto& e){ handle_wheel_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_SAVER_ACTIVATED,
                          m_primaryClient->getEventTarget(),
                          [this](const auto& e){ handle_screensaver_activated_event(); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_SAVER_DEACTIVATED,
                          m_primaryClient->getEventTarget(),
                          [this](const auto& e){ handle_screensaver_deactivated_event(); });
    m_events->add_handler(EventType::SERVER_SWITCH_TO_SCREEN, m_inputFilter,
                          [this](const auto& e){ handle_switch_to_screen_event(e); });
    m_events->add_handler(EventType::SERVER_TOGGLE_SCREEN, m_inputFilter,
                          [this](const auto& e){ handle_toggle_screen_event(e); });
    m_events->add_handler(EventType::SERVER_SWITCH_INDIRECTION, m_inputFilter,
                          [this](const auto& e){ handle_switch_in_direction_event(e); });
    m_events->add_handler(EventType::SERVER_KEYBOARD_BROADCAST, m_inputFilter,
                          [this](const auto& e){ handle_keyboard_broadcast_event(e); });
    m_events->add_handler(EventType::SERVER_LOCK_CURSOR_TO_SCREEN, m_inputFilter,
                          [this](const auto& e){ handle_lock_cursor_to_screen_event(e); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_FAKE_INPUT_BEGIN, m_inputFilter,
                          [this](const auto& e){ handle_fake_input_begin_event(); });
    m_events->add_handler(EventType::PRIMARY_SCREEN_FAKE_INPUT_END, m_inputFilter,
                          [this](const auto& e){ handle_fake_input_end_event(); });

    if (m_args.m_enableDragDrop) {
        m_events->add_handler(EventType::FILE_CHUNK_SENDING, this,
                              [this](const auto& e){ handle_file_chunk_sending_event(e); });
        m_events->add_handler(EventType::FILE_RECEIVE_COMPLETED, this,
                              [this](const auto& e){ handle_file_receive_completed_event(e); });
	}

	// add connection
	addClient(m_primaryClient);

	// set initial configuration
	setConfig(config);

	// enable primary client
	m_primaryClient->enable();
	m_inputFilter->setPrimaryClient(m_primaryClient);

	// Determine if scroll lock is already set. If so, lock the cursor to the primary screen
	if (m_primaryClient->getToggleMask() & KeyModifierScrollLock) {
		LOG((CLOG_NOTE "Scroll Lock is on, locking cursor to screen"));
		m_lockedToScreen = true;
	}

}

Server::~Server()
{
	if (m_mock) {
		return;
	}

	// remove event handlers and timers
    m_events->removeHandler(EventType::KEY_STATE_KEY_DOWN, m_inputFilter);
    m_events->removeHandler(EventType::KEY_STATE_KEY_UP, m_inputFilter);
    m_events->removeHandler(EventType::KEY_STATE_KEY_REPEAT, m_inputFilter);
    m_events->removeHandler(EventType::PRIMARY_SCREEN_BUTTON_DOWN, m_inputFilter);
    m_events->removeHandler(EventType::PRIMARY_SCREEN_BUTTON_UP, m_inputFilter);
    m_events->removeHandler(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY, m_primaryClient->getEventTarget());
    m_events->removeHandler(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY, m_primaryClient->getEventTarget());
    m_events->removeHandler(EventType::PRIMARY_SCREEN_WHEEL, m_primaryClient->getEventTarget());
    m_events->removeHandler(EventType::PRIMARY_SCREEN_SAVER_ACTIVATED, m_primaryClient->getEventTarget());
    m_events->removeHandler(EventType::PRIMARY_SCREEN_SAVER_DEACTIVATED, m_primaryClient->getEventTarget());
    m_events->removeHandler(EventType::PRIMARY_SCREEN_FAKE_INPUT_BEGIN, m_inputFilter);
    m_events->removeHandler(EventType::PRIMARY_SCREEN_FAKE_INPUT_END, m_inputFilter);
    m_events->removeHandler(EventType::TIMER, this);
	stopSwitch();

	// force immediate disconnection of secondary clients
	disconnect();
	for (OldClients::iterator index = m_oldClients.begin();
							index != m_oldClients.end(); ++index) {
		BaseClientProxy* client = index->first;
		m_events->deleteTimer(index->second);
        m_events->removeHandler(EventType::TIMER, client);
        m_events->removeHandler(EventType::CLIENT_PROXY_DISCONNECTED, client);
		delete client;
	}

	// remove input filter
	m_inputFilter->setPrimaryClient(nullptr);

	// disable and disconnect primary client
	m_primaryClient->disable();
	removeClient(m_primaryClient);
}

bool
Server::setConfig(const Config& config)
{
	// refuse configuration if it doesn't include the primary screen
	if (!config.isScreen(m_primaryClient->getName())) {
		return false;
	}

	// close clients that are connected but being dropped from the
	// configuration.
	closeClients(config);

	// cut over
	processOptions();

	// add ScrollLock as a hotkey to lock to the screen.  this was a
	// built-in feature in earlier releases and is now supported via
	// the user configurable hotkey mechanism.  if the user has already
	// registered ScrollLock for something else then that will win but
	// we will unfortunately generate a warning.  if the user has
	// configured a LockCursorToScreenAction then we don't add
	// ScrollLock as a hotkey.
	if (!m_config->hasLockToScreenAction()) {
        IPlatformScreen::KeyInfo key{kKeyScrollLock, 0, 0, 0};
		InputFilter::Rule rule(new InputFilter::KeystrokeCondition(m_events, key));
		rule.adoptAction(new InputFilter::LockCursorToScreenAction(m_events), true);
		m_inputFilter->addFilterRule(rule);
	}

	// tell primary screen about reconfiguration
	m_primaryClient->reconfigure(getActivePrimarySides());

	// tell all (connected) clients about current options
	for (ClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		BaseClientProxy* client = index->second;
		sendOptions(client);
	}

	return true;
}

void
Server::adoptClient(BaseClientProxy* client)
{
	assert(client != nullptr);

	// watch for client disconnection
    m_events->add_handler(EventType::CLIENT_PROXY_DISCONNECTED, client,
                          [this, client](const auto& e){ handle_client_disconnected(client); });

	// name must be in our configuration
    if (!m_config->isScreen(client->getName())) {
		LOG((CLOG_WARN "unrecognised client name \"%s\", check server config", client->getName().c_str()));
		closeClient(client, kMsgEUnknown);
		return;
	}

	// add client to client list
	if (!addClient(client)) {
		// can only have one screen with a given name at any given time
		LOG((CLOG_WARN "a client with name \"%s\" is already connected", getName(client).c_str()));
		closeClient(client, kMsgEBusy);
		return;
	}
	LOG((CLOG_NOTE "client \"%s\" has connected", getName(client).c_str()));

	// send configuration options to client
	sendOptions(client);

	// activate screen saver on new client if active on the primary screen
	if (m_activeSaver != nullptr) {
		client->screensaver(true);
	}

	// send notification
    Server::ScreenConnectedInfo info{getName(client)};
    m_events->add_event(EventType::SERVER_CONNECTED, m_primaryClient->getEventTarget(),
                        create_event_data<Server::ScreenConnectedInfo>(info));
}

void
Server::disconnect()
{
	// close all secondary clients
	if (m_clients.size() > 1 || !m_oldClients.empty()) {
		Config emptyConfig(m_events);
		closeClients(emptyConfig);
	}
	else {
        m_events->add_event(EventType::SERVER_DISCONNECTED, this);
	}
}

std::uint32_t Server::getNumClients() const
{
    return static_cast<std::int32_t>(m_clients.size());
}

void
Server::getClients(std::vector<std::string>& list) const
{
	list.clear();
	for (ClientList::const_iterator index = m_clients.begin();
							index != m_clients.end(); ++index) {
		list.push_back(index->first);
	}
}

std::string Server::getName(const BaseClientProxy* client) const
{
    std::string name = m_config->getCanonicalName(client->getName());
	if (name.empty()) {
		name = client->getName();
	}
	return name;
}

std::uint32_t Server::getActivePrimarySides() const
{
    std::uint32_t sides = 0;
	if (!isLockedToScreenServer()) {
		if (hasAnyNeighbor(m_primaryClient, kLeft)) {
			sides |= kLeftMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kRight)) {
			sides |= kRightMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kTop)) {
			sides |= kTopMask;
		}
		if (hasAnyNeighbor(m_primaryClient, kBottom)) {
			sides |= kBottomMask;
		}
	}
	return sides;
}

bool
Server::isLockedToScreenServer() const
{
	// locked if scroll-lock is toggled on
	return m_lockedToScreen;
}

bool
Server::isLockedToScreen() const
{
	// locked if we say we're locked
	if (isLockedToScreenServer()) {
		return true;
	}

	// locked if primary says we're locked
	if (m_primaryClient->isLockedToScreen()) {
		return true;
	}

	// not locked
	return false;
}

std::int32_t Server::getJumpZoneSize(BaseClientProxy* client) const
{
	if (client == m_primaryClient) {
		return m_primaryClient->getJumpZoneSize();
	}
	else {
		return 0;
	}
}

void Server::switchScreen(BaseClientProxy* dst, std::int32_t x, std::int32_t y, bool forScreensaver)
{
	assert(dst != nullptr);

#ifndef NDEBUG
	{
		std::int32_t dx, dy, dw, dh;
		dst->getShape(dx, dy, dw, dh);
		assert(x >= dx && y >= dy && x < dx + dw && y < dy + dh);
	}
#endif
	assert(m_active != nullptr);

	LOG((CLOG_INFO "switch from \"%s\" to \"%s\" at %d,%d", getName(m_active).c_str(), getName(dst).c_str(), x, y));

	// stop waiting to switch
	stopSwitch();

	// record new position
	m_x       = x;
	m_y       = y;
	m_xDelta  = 0;
	m_yDelta  = 0;
	m_xDelta2 = 0;
	m_yDelta2 = 0;

	// wrapping means leaving the active screen and entering it again.
	// since that's a waste of time we skip that and just warp the
	// mouse.
	if (m_active != dst) {
		// leave active screen
		if (!m_active->leave()) {
			// cannot leave screen
			LOG((CLOG_WARN "can't leave screen"));
			return;
		}

		// update the primary client's clipboards if we're leaving the
		// primary screen.
		if (m_active == m_primaryClient && m_enableClipboard) {
			for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
				ClipboardInfo& clipboard = m_clipboards[id];
				if (clipboard.m_clipboardOwner == getName(m_primaryClient)) {
					onClipboardChanged(m_primaryClient,
						id, clipboard.m_clipboardSeqNum);
				}
			}
		}

		// cut over
		m_active = dst;

		// increment enter sequence number
		++m_seqNum;

		// enter new screen
		m_active->enter(x, y, m_seqNum,
								m_primaryClient->getToggleMask(),
								forScreensaver);

		if (m_enableClipboard) {
			// send the clipboard data to new active screen
			for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
				m_active->setClipboard(id, &m_clipboards[id].m_clipboard);
			}
		}

        Server::SwitchToScreenInfo info{m_active->getName()};
        m_events->add_event(EventType::SERVER_SCREEN_SWITCHED, this,
                            create_event_data<Server::SwitchToScreenInfo>(info));
	}
	else {
		m_active->mouseMove(x, y);
	}
}

void
Server::jumpToScreen(BaseClientProxy* newScreen)
{
	assert(newScreen != nullptr);

	// record the current cursor position on the active screen
	m_active->setJumpCursorPos(m_x, m_y);

	// get the last cursor position on the target screen
	std::int32_t x, y;
	newScreen->getJumpCursorPos(x, y);

	switchScreen(newScreen, x, y, false);
}

float Server::mapToFraction(BaseClientProxy* client, EDirection dir, std::int32_t x,
                            std::int32_t y) const
{
	std::int32_t sx, sy, sw, sh;
	client->getShape(sx, sy, sw, sh);
	switch (dir) {
	case kLeft:
	case kRight:
		return static_cast<float>(y - sy + 0.5f) / static_cast<float>(sh);

	case kTop:
	case kBottom:
		return static_cast<float>(x - sx + 0.5f) / static_cast<float>(sw);

	case kNoDirection:
		assert(0 && "bad direction");
		break;
    default:
        break;
	}
	return 0.0f;
}

void Server::mapToPixel(BaseClientProxy* client, EDirection dir, float f, std::int32_t& x,
                        std::int32_t& y) const
{
	std::int32_t sx, sy, sw, sh;
	client->getShape(sx, sy, sw, sh);
	switch (dir) {
	case kLeft:
	case kRight:
		y = static_cast<std::int32_t>(f * sh) + sy;
		break;

	case kTop:
	case kBottom:
		x = static_cast<std::int32_t>(f * sw) + sx;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
		break;
    default:
        break;
	}
}

bool
Server::hasAnyNeighbor(BaseClientProxy* client, EDirection dir) const
{
	assert(client != nullptr);

	return m_config->hasNeighbor(getName(client), dir);
}

BaseClientProxy* Server::getNeighbor(BaseClientProxy* src, EDirection dir, std::int32_t& x,
                                     std::int32_t& y) const
{
	// note -- must be locked on entry

	assert(src != nullptr);

	// get source screen name
    std::string srcName = getName(src);
	assert(!srcName.empty());
	LOG((CLOG_DEBUG2 "find neighbor on %s of \"%s\"", Config::dirName(dir), srcName.c_str()));

	// convert position to fraction
	float t = mapToFraction(src, dir, x, y);

	// search for the closest neighbor that exists in direction dir
	float tTmp;
	for (;;) {
        std::string dstName(m_config->getNeighbor(srcName, dir, t, &tTmp));

		// if nothing in that direction then return nullptr. if the
		// destination is the source then we can make no more
		// progress in this direction.  since we haven't found a
		// connected neighbor we return nullptr.
		if (dstName.empty()) {
			LOG((CLOG_DEBUG2 "no neighbor on %s of \"%s\"", Config::dirName(dir), srcName.c_str()));
			return nullptr;
		}

		// look up neighbor cell.  if the screen is connected and
		// ready then we can stop.
		ClientList::const_iterator index = m_clients.find(dstName);
		if (index != m_clients.end()) {
			LOG((CLOG_DEBUG2 "\"%s\" is on %s of \"%s\" at %f", dstName.c_str(), Config::dirName(dir), srcName.c_str(), t));
			mapToPixel(index->second, dir, tTmp, x, y);
			return index->second;
		}

		// skip over unconnected screen
		LOG((CLOG_DEBUG2 "ignored \"%s\" on %s of \"%s\"", dstName.c_str(), Config::dirName(dir), srcName.c_str()));
		srcName = dstName;

		// use position on skipped screen
		t = tTmp;
	}
}

BaseClientProxy* Server::mapToNeighbor(BaseClientProxy* src, EDirection srcSide, std::int32_t& x,
                                       std::int32_t& y) const
{
	// note -- must be locked on entry

	assert(src != nullptr);

	// get the first neighbor
	BaseClientProxy* dst = getNeighbor(src, srcSide, x, y);
	if (dst == nullptr) {
		return nullptr;
	}

	// get the source screen's size
	std::int32_t dx, dy, dw, dh;
	BaseClientProxy* lastGoodScreen = src;
	lastGoodScreen->getShape(dx, dy, dw, dh);

	// find destination screen, adjusting x or y (but not both).  the
	// searches are done in a sort of canonical screen space where
	// the upper-left corner is 0,0 for each screen.  we adjust from
	// actual to canonical position on entry to and from canonical to
	// actual on exit from the search.
	switch (srcSide) {
	case kLeft:
		x -= dx;
		while (dst != nullptr) {
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			x += dw;
			if (x >= 0) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
		}
		assert(lastGoodScreen != nullptr);
		x += dx;
		break;

	case kRight:
		x -= dx;
		while (dst != nullptr) {
			x -= dw;
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			if (x < dw) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
		}
		assert(lastGoodScreen != nullptr);
		x += dx;
		break;

	case kTop:
		y -= dy;
		while (dst != nullptr) {
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			y += dh;
			if (y >= 0) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
		}
		assert(lastGoodScreen != nullptr);
		y += dy;
		break;

	case kBottom:
		y -= dy;
		while (dst != nullptr) {
			y -= dh;
			lastGoodScreen = dst;
			lastGoodScreen->getShape(dx, dy, dw, dh);
			if (y < dh) {
				break;
			}
			LOG((CLOG_DEBUG2 "skipping over screen %s", getName(dst).c_str()));
			dst = getNeighbor(lastGoodScreen, srcSide, x, y);
		}
		assert(lastGoodScreen != nullptr);
		y += dy;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
		return nullptr;
    default:
        break;
	}

	// save destination screen
	assert(lastGoodScreen != nullptr);
	dst = lastGoodScreen;

	// if entering primary screen then be sure to move in far enough
	// to avoid the jump zone.  if entering a side that doesn't have
	// a neighbor (i.e. an asymmetrical side) then we don't need to
	// move inwards because that side can't provoke a jump.
	avoidJumpZone(dst, srcSide, x, y);

	return dst;
}

void Server::avoidJumpZone(BaseClientProxy* dst, EDirection dir, std::int32_t& x,
                           std::int32_t& y) const
{
	// we only need to avoid jump zones on the primary screen
	if (dst != m_primaryClient) {
		return;
	}

    const std::string dstName(getName(dst));
	std::int32_t dx, dy, dw, dh;
	dst->getShape(dx, dy, dw, dh);
	float t = mapToFraction(dst, dir, x, y);
	std::int32_t z = getJumpZoneSize(dst);

	// move in far enough to avoid the jump zone.  if entering a side
	// that doesn't have a neighbor (i.e. an asymmetrical side) then we
	// don't need to move inwards because that side can't provoke a jump.
	switch (dir) {
	case kLeft:
		if (!m_config->getNeighbor(dstName, kRight, t, nullptr).empty() &&
			x > dx + dw - 1 - z)
			x = dx + dw - 1 - z;
		break;

	case kRight:
		if (!m_config->getNeighbor(dstName, kLeft, t, nullptr).empty() &&
			x < dx + z)
			x = dx + z;
		break;

	case kTop:
		if (!m_config->getNeighbor(dstName, kBottom, t, nullptr).empty() &&
			y > dy + dh - 1 - z)
			y = dy + dh - 1 - z;
		break;

	case kBottom:
		if (!m_config->getNeighbor(dstName, kTop, t, nullptr).empty() &&
			y < dy + z)
			y = dy + z;
		break;

	case kNoDirection:
		assert(0 && "bad direction");
    default:
        break;
	}
}

bool Server::isSwitchOkay(BaseClientProxy* newScreen, EDirection dir, std::int32_t x,
                          std::int32_t y, std::int32_t xActive, std::int32_t yActive)
{
	LOG((CLOG_DEBUG1 "try to leave \"%s\" on %s", getName(m_active).c_str(), Config::dirName(dir)));

	// is there a neighbor?
	if (newScreen == nullptr) {
		// there's no neighbor.  we don't want to switch and we don't
		// want to try to switch later.
		LOG((CLOG_DEBUG1 "no neighbor %s", Config::dirName(dir)));
		stopSwitch();
		return false;
	}

	// should we switch or not?
	bool preventSwitch = false;
	bool allowSwitch   = false;

	// note if the switch direction has changed.  save the new
	// direction and screen if so.
	bool isNewDirection  = (dir != m_switchDir);
	if (isNewDirection || m_switchScreen == nullptr) {
		m_switchDir    = dir;
		m_switchScreen = newScreen;
	}

	// is this a double tap and do we care?
	if (!allowSwitch && m_switchTwoTapDelay > 0.0) {
		if (isNewDirection ||
			!isSwitchTwoTapStarted() || !shouldSwitchTwoTap()) {
			// tapping a different or new edge or second tap not
			// fast enough.  prepare for second tap.
			preventSwitch = true;
			startSwitchTwoTap();
		}
		else {
			// got second tap
			allowSwitch = true;
		}
	}

	// if waiting before a switch then prepare to switch later
	if (!allowSwitch && m_switchWaitDelay > 0.0) {
		if (isNewDirection || !isSwitchWaitStarted()) {
			startSwitchWait(x, y);
		}
		preventSwitch = true;
	}

	// are we in a locked corner?  first check if screen has the option set
	// and, if not, check the global options.
	const Config::ScreenOptions* options =
						m_config->getOptions(getName(m_active));
	if (options == nullptr || options->count(kOptionScreenSwitchCorners) == 0) {
		options = m_config->getOptions("");
	}
	if (options != nullptr && options->count(kOptionScreenSwitchCorners) > 0) {
		// get corner mask and size
		Config::ScreenOptions::const_iterator i =
			options->find(kOptionScreenSwitchCorners);
        std::uint32_t corners = static_cast<std::uint32_t>(i->second);
		i = options->find(kOptionScreenSwitchCornerSize);
		std::int32_t size = 0;
		if (i != options->end()) {
			size = i->second;
		}

		// see if we're in a locked corner
		if ((getCorner(m_active, xActive, yActive, size) & corners) != 0) {
			// yep, no switching
			LOG((CLOG_DEBUG1 "locked in corner"));
			preventSwitch = true;
			stopSwitch();
		}
	}

	// ignore if mouse is locked to screen and don't try to switch later
	if (!preventSwitch && isLockedToScreen()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		preventSwitch = true;
		stopSwitch();
	}

	// check for optional needed modifiers
	KeyModifierMask mods = this->m_primaryClient->getToggleMask();

	if (!preventSwitch && (
			(this->m_switchNeedsShift && ((mods & KeyModifierShift) != KeyModifierShift)) ||
			(this->m_switchNeedsControl && ((mods & KeyModifierControl) != KeyModifierControl)) ||
			(this->m_switchNeedsAlt && ((mods & KeyModifierAlt) != KeyModifierAlt))
		)) {
		LOG((CLOG_DEBUG1 "need modifiers to switch"));
		preventSwitch = true;
		stopSwitch();
	}

	return !preventSwitch;
}

void Server::noSwitch(std::int32_t x, std::int32_t y)
{
	armSwitchTwoTap(x, y);
	stopSwitchWait();
}

void
Server::stopSwitch()
{
	if (m_switchScreen != nullptr) {
		m_switchScreen = nullptr;
		m_switchDir    = kNoDirection;
		stopSwitchTwoTap();
		stopSwitchWait();
	}
}

void
Server::startSwitchTwoTap()
{
	m_switchTwoTapEngaged = true;
	m_switchTwoTapArmed   = false;
	m_switchTwoTapTimer.reset();
	LOG((CLOG_DEBUG1 "waiting for second tap"));
}

void Server::armSwitchTwoTap(std::int32_t x, std::int32_t y)
{
	if (m_switchTwoTapEngaged) {
		if (m_switchTwoTapTimer.getTime() > m_switchTwoTapDelay) {
			// second tap took too long.  disengage.
			stopSwitchTwoTap();
		}
		else if (!m_switchTwoTapArmed) {
			// still time for a double tap.  see if we left the tap
			// zone and, if so, arm the two tap.
			std::int32_t ax, ay, aw, ah;
			m_active->getShape(ax, ay, aw, ah);
			std::int32_t tapZone = m_primaryClient->getJumpZoneSize();
			if (tapZone < m_switchTwoTapZone) {
				tapZone = m_switchTwoTapZone;
			}
			if (x >= ax + tapZone && x < ax + aw - tapZone &&
				y >= ay + tapZone && y < ay + ah - tapZone) {
				// win32 can generate bogus mouse events that appear to
				// move in the opposite direction that the mouse actually
				// moved.  try to ignore that crap here.
				switch (m_switchDir) {
				case kLeft:
					m_switchTwoTapArmed = (m_xDelta > 0 && m_xDelta2 > 0);
					break;

				case kRight:
					m_switchTwoTapArmed = (m_xDelta < 0 && m_xDelta2 < 0);
					break;

				case kTop:
					m_switchTwoTapArmed = (m_yDelta > 0 && m_yDelta2 > 0);
					break;

				case kBottom:
					m_switchTwoTapArmed = (m_yDelta < 0 && m_yDelta2 < 0);
					break;

				default:
					break;
				}
			}
		}
	}
}

void
Server::stopSwitchTwoTap()
{
	m_switchTwoTapEngaged = false;
	m_switchTwoTapArmed   = false;
}

bool
Server::isSwitchTwoTapStarted() const
{
	return m_switchTwoTapEngaged;
}

bool
Server::shouldSwitchTwoTap() const
{
	// this is the second tap if two-tap is armed and this tap
	// came fast enough
	return (m_switchTwoTapArmed &&
			m_switchTwoTapTimer.getTime() <= m_switchTwoTapDelay);
}

void
Server::startSwitchWait(std::int32_t x, std::int32_t y)
{
	stopSwitchWait();
	m_switchWaitX     = x;
	m_switchWaitY     = y;
	m_switchWaitTimer = m_events->newOneShotTimer(m_switchWaitDelay, this);
	LOG((CLOG_DEBUG1 "waiting to switch"));
}

void
Server::stopSwitchWait()
{
	if (m_switchWaitTimer != nullptr) {
		m_events->deleteTimer(m_switchWaitTimer);
		m_switchWaitTimer = nullptr;
	}
}

bool
Server::isSwitchWaitStarted() const
{
	return (m_switchWaitTimer != nullptr);
}

std::uint32_t Server::getCorner(BaseClientProxy* client, std::int32_t x, std::int32_t y,
                                std::int32_t size) const
{
	assert(client != nullptr);

	// get client screen shape
	std::int32_t ax, ay, aw, ah;
	client->getShape(ax, ay, aw, ah);

	// check for x,y on the left or right
	std::int32_t xSide;
	if (x <= ax) {
		xSide = -1;
	}
	else if (x >= ax + aw - 1) {
		xSide = 1;
	}
	else {
		xSide = 0;
	}

	// check for x,y on the top or bottom
	std::int32_t ySide;
	if (y <= ay) {
		ySide = -1;
	}
	else if (y >= ay + ah - 1) {
		ySide = 1;
	}
	else {
		ySide = 0;
	}

	// if against the left or right then check if y is within size
	if (xSide != 0) {
		if (y < ay + size) {
			return (xSide < 0) ? kTopLeftMask : kTopRightMask;
		}
		else if (y >= ay + ah - size) {
			return (xSide < 0) ? kBottomLeftMask : kBottomRightMask;
		}
	}

	// if against the left or right then check if y is within size
	if (ySide != 0) {
		if (x < ax + size) {
			return (ySide < 0) ? kTopLeftMask : kBottomLeftMask;
		}
		else if (x >= ax + aw - size) {
			return (ySide < 0) ? kTopRightMask : kBottomRightMask;
		}
	}

	return kNoCornerMask;
}

void
Server::stopRelativeMoves()
{
	if (m_relativeMoves && m_active != m_primaryClient) {
		// warp to the center of the active client so we know where we are
		std::int32_t ax, ay, aw, ah;
		m_active->getShape(ax, ay, aw, ah);
		m_x       = ax + (aw >> 1);
		m_y       = ay + (ah >> 1);
		m_xDelta  = 0;
		m_yDelta  = 0;
		m_xDelta2 = 0;
		m_yDelta2 = 0;
		LOG((CLOG_DEBUG2 "synchronize move on %s by %d,%d", getName(m_active).c_str(), m_x, m_y));
		m_active->mouseMove(m_x, m_y);
	}
}

void
Server::sendOptions(BaseClientProxy* client) const
{
	OptionsList optionsList;

	// look up options for client
	const Config::ScreenOptions* options =
						m_config->getOptions(getName(client));
	if (options != nullptr) {
		// convert options to a more convenient form for sending
		optionsList.reserve(2 * options->size());
		for (Config::ScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
            optionsList.push_back(static_cast<std::uint32_t>(index->second));
		}
	}

	// look up global options
	options = m_config->getOptions("");
	if (options != nullptr) {
		// convert options to a more convenient form for sending
		optionsList.reserve(optionsList.size() + 2 * options->size());
		for (Config::ScreenOptions::const_iterator index = options->begin();
									index != options->end(); ++index) {
			optionsList.push_back(index->first);
            optionsList.push_back(static_cast<std::uint32_t>(index->second));
		}
	}

	// send the options
	client->resetOptions();
	client->setOptions(optionsList);
}

void
Server::processOptions()
{
	const Config::ScreenOptions* options = m_config->getOptions("");
	if (options == nullptr) {
		return;
	}

	m_switchNeedsShift = false;		// it seems if I don't add these
	m_switchNeedsControl = false;	// lines, the 'reload config' option
	m_switchNeedsAlt = false;		// doesn't work correct.

	bool newRelativeMoves = m_relativeMoves;
	for (Config::ScreenOptions::const_iterator index = options->begin();
								index != options->end(); ++index) {
		const OptionID id       = index->first;
		const OptionValue value = index->second;
		if (id == kOptionScreenSwitchDelay) {
			m_switchWaitDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchWaitDelay < 0.0) {
				m_switchWaitDelay = 0.0;
			}
			stopSwitchWait();
		}
		else if (id == kOptionScreenSwitchTwoTap) {
			m_switchTwoTapDelay = 1.0e-3 * static_cast<double>(value);
			if (m_switchTwoTapDelay < 0.0) {
				m_switchTwoTapDelay = 0.0;
			}
			stopSwitchTwoTap();
		}
		else if (id == kOptionScreenSwitchNeedsControl) {
			m_switchNeedsControl = (value != 0);
		}
		else if (id == kOptionScreenSwitchNeedsShift) {
			m_switchNeedsShift = (value != 0);
		}
		else if (id == kOptionScreenSwitchNeedsAlt) {
			m_switchNeedsAlt = (value != 0);
		}
		else if (id == kOptionRelativeMouseMoves) {
			newRelativeMoves = (value != 0);
		}
		else if (id == kOptionClipboardSharing) {
			m_enableClipboard = (value != 0);

			if (m_enableClipboard == false) {
				LOG((CLOG_NOTE "clipboard sharing is disabled"));
			}
		}
	}
	if (m_relativeMoves && !newRelativeMoves) {
		stopRelativeMoves();
	}
	m_relativeMoves = newRelativeMoves;
}

void Server::handle_shape_changed(BaseClientProxy* client)
{
	// ignore events from unknown clients
	if (m_clientSet.count(client) == 0) {
		return;
	}

	LOG((CLOG_DEBUG "screen \"%s\" shape changed", getName(client).c_str()));

	// update jump coordinate
	std::int32_t x, y;
	client->getCursorPos(x, y);
	client->setJumpCursorPos(x, y);

	// update the mouse coordinates
	if (client == m_active) {
		m_x = x;
		m_y = y;
	}

	// handle resolution change to primary screen
	if (client == m_primaryClient) {
		if (client == m_active) {
			onMouseMovePrimary(m_x, m_y);
		}
		else {
			onMouseMoveSecondary(0, 0);
		}
	}
}

void Server::handle_clipboard_grabbed(const Event& event, BaseClientProxy* grabber)
{
	if (!m_enableClipboard) {
		return;
	}

	// ignore events from unknown clients
	if (m_clientSet.count(grabber) == 0) {
		return;
	}
  // ignore grab event from non-active client, which happens if their clipboard is updated in background
	if (grabber != m_active) {
		return;
	}
    const auto& info = event.get_data_as<IScreen::ClipboardInfo>();

	// ignore grab if sequence number is old.  always allow primary
	// screen to grab.
    ClipboardInfo& clipboard = m_clipboards[info.m_id];
    if (grabber != m_primaryClient && info.m_sequenceNumber < clipboard.m_clipboardSeqNum) {
        LOG((CLOG_INFO "ignored screen \"%s\" grab of clipboard %d", getName(grabber).c_str(),
             info.m_id));
		return;
	}

	// mark screen as owning clipboard
    LOG((CLOG_INFO "screen \"%s\" grabbed clipboard %d from \"%s\"", getName(grabber).c_str(),
         info.m_id, clipboard.m_clipboardOwner.c_str()));
	clipboard.m_clipboardOwner  = getName(grabber);
    clipboard.m_clipboardSeqNum = info.m_sequenceNumber;

	// clear the clipboard data (since it's not known at this point)
	if (clipboard.m_clipboard.open(0)) {
		clipboard.m_clipboard.empty();
		clipboard.m_clipboard.close();
	}
	clipboard.m_clipboardData = clipboard.m_clipboard.marshall();

	// tell all other screens to take ownership of clipboard.  tell the
	// grabber that it's clipboard isn't dirty.
	for (ClientList::iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		BaseClientProxy* client = index->second;
		if (client == grabber) {
            client->setClipboardDirty(info.m_id, false);
		}
		else {
            client->grabClipboard(info.m_id);
		}
	}
}

void Server::handle_clipboard_changed(const Event& event, BaseClientProxy* client)
{
	// ignore events from unknown clients
    if (m_clientSet.count(client) == 0) {
		return;
	}
    const auto& info = event.get_data_as<IScreen::ClipboardInfo>();
    onClipboardChanged(client, info.m_id, info.m_sequenceNumber);
}

void Server::handle_key_down_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::KeyInfo>();
    onKeyDown(info.m_key, info.m_mask, info.m_button, info.screens_or_nullptr());
}

void Server::handle_key_up_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::KeyInfo>();
    onKeyUp(info.m_key, info.m_mask, info.m_button, info.screens_or_nullptr());
}

void Server::handle_key_repeat_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::KeyInfo>();
    onKeyRepeat(info.m_key, info.m_mask, info.m_count, info.m_button);
}

void Server::handle_button_down_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::ButtonInfo>();
    onMouseDown(info.m_button);
}

void Server::handle_button_up_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::ButtonInfo>();
    onMouseUp(info.m_button);
}

void Server::handle_motion_primary_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::MotionInfo>();
    onMouseMovePrimary(info.m_x, info.m_y);
}

void Server::handle_motion_secondary_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::MotionInfo>();
    onMouseMoveSecondary(info.m_x, info.m_y);
}

void Server::handle_wheel_event(const Event& event)
{
    const auto& info = event.get_data_as<IPlatformScreen::WheelInfo>();
    onMouseWheel(info.m_xDelta, info.m_yDelta);
}

void Server::handle_screensaver_activated_event()
{
	onScreensaver(true);
}

void Server::handle_screensaver_deactivated_event()
{
	onScreensaver(false);
}

void Server::handle_switch_wait_event()
{
	// ignore if mouse is locked to screen
	if (isLockedToScreen()) {
		LOG((CLOG_DEBUG1 "locked to screen"));
		stopSwitch();
		return;
	}

	// switch screen
	switchScreen(m_switchScreen, m_switchWaitX, m_switchWaitY, false);
}

void Server::handle_client_disconnected(BaseClientProxy* client)
{
	// client has disconnected.  it might be an old client or an
	// active client.  we don't care so just handle it both ways.
	removeActiveClient(client);
	removeOldClient(client);

	delete client;
}

void Server::handle_client_close_timeout(BaseClientProxy* client)
{
	// client took too long to disconnect.  just dump it.
	LOG((CLOG_NOTE "forced disconnection of client \"%s\"", getName(client).c_str()));
	removeOldClient(client);

	delete client;
}

void Server::handle_switch_to_screen_event(const Event& event)
{
    const auto& info = event.get_data_as<SwitchToScreenInfo>();

    ClientList::const_iterator index = m_clients.find(info.m_screen);
	if (index == m_clients.end()) {
        LOG((CLOG_DEBUG1 "screen \"%s\" not active", info.m_screen.c_str()));
	}
	else {
        jumpToScreen(index->second);
	}
}

void
Server::handle_toggle_screen_event(const Event& event)
{
    (void) event;

  std::string current = getName(m_active);
  ClientList::const_iterator index = m_clients.find(current);
  if (index == m_clients.end()) {
    LOG((CLOG_DEBUG1 "screen \"%s\" not active", current.c_str()));
  }
  else {
    ++index;
    if (index == m_clients.end()) {
      index = m_clients.begin();
    }
    jumpToScreen(index->second);
  }
}


void Server::handle_switch_in_direction_event(const Event& event)
{
    const auto& info = event.get_data_as<SwitchInDirectionInfo>();

	// jump to screen in chosen direction from center of this screen
	std::int32_t x = m_x, y = m_y;
    BaseClientProxy* newScreen = getNeighbor(m_active, info.m_direction, x, y);
	if (newScreen == nullptr) {
        LOG((CLOG_DEBUG1 "no neighbor %s", Config::dirName(info.m_direction)));
	}
	else {
		jumpToScreen(newScreen);
	}
}

void Server::handle_keyboard_broadcast_event(const Event& event)
{
    const auto& info = event.get_data_as<KeyboardBroadcastInfo>();

	// choose new state
	bool newState;
    switch (info.m_state) {
	case KeyboardBroadcastInfo::kOff:
		newState = false;
		break;

	default:
	case KeyboardBroadcastInfo::kOn:
		newState = true;
		break;

	case KeyboardBroadcastInfo::kToggle:
		newState = !m_keyboardBroadcasting;
		break;
	}

	// enter new state
	if (newState != m_keyboardBroadcasting ||
        info.screens_ != m_keyboardBroadcastingScreens) {
		m_keyboardBroadcasting        = newState;
        m_keyboardBroadcastingScreens = info.screens_;
		LOG((CLOG_DEBUG "keyboard broadcasting %s: %s", m_keyboardBroadcasting ? "on" : "off", m_keyboardBroadcastingScreens.c_str()));
	}
}

void Server::handle_lock_cursor_to_screen_event(const Event& event)
{
    const auto& info = event.get_data_as<LockCursorToScreenInfo>();

	// choose new state
	bool newState;
    switch (info.m_state) {
	case LockCursorToScreenInfo::kOff:
		newState = false;
		break;

	default:
	case LockCursorToScreenInfo::kOn:
		newState = true;
		break;

	case LockCursorToScreenInfo::kToggle:
		newState = !m_lockedToScreen;
		break;
	}

	// enter new state
	if (newState != m_lockedToScreen) {
		m_lockedToScreen = newState;
		LOG((CLOG_NOTE "cursor %s current screen", m_lockedToScreen ? "locked to" : "unlocked from"));

		m_primaryClient->reconfigure(getActivePrimarySides());
		if (!isLockedToScreenServer()) {
			stopRelativeMoves();
		}
	}
}

void Server::handle_fake_input_begin_event()
{
	m_primaryClient->fakeInputBegin();
}

void Server::handle_fake_input_end_event()
{
	m_primaryClient->fakeInputEnd();
}

void Server::handle_file_chunk_sending_event(const Event& event)
{
    on_file_chunk_sending(event.get_data_as<FileChunk>());
}

void Server::handle_file_receive_completed_event(const Event& event)
{
    (void) event;

	onFileReceiveCompleted();
}

void Server::onClipboardChanged(BaseClientProxy* sender, ClipboardID id, std::uint32_t seqNum)
{
	ClipboardInfo& clipboard = m_clipboards[id];

	// ignore update if sequence number is old
	if (seqNum < clipboard.m_clipboardSeqNum) {
		LOG((CLOG_INFO "ignored screen \"%s\" update of clipboard %d (missequenced)", getName(sender).c_str(), id));
		return;
	}

	// should be the expected client
	assert(sender == m_clients.find(clipboard.m_clipboardOwner)->second);

	// get data
	if (!sender->getClipboard(id, &clipboard.m_clipboard)) {
		LOG((CLOG_DEBUG "ignored screen \"%s\" update of clipboard %d (failed to get clipboard)",
				clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// ignore if data hasn't changed
    std::string data = clipboard.m_clipboard.marshall();
	if (data == clipboard.m_clipboardData) {
		LOG((CLOG_DEBUG "ignored screen \"%s\" update of clipboard %d (unchanged)", clipboard.m_clipboardOwner.c_str(), id));
		return;
	}

	// got new data
	LOG((CLOG_INFO "screen \"%s\" updated clipboard %d", clipboard.m_clipboardOwner.c_str(), id));
	clipboard.m_clipboardData = data;

	// tell all clients except the sender that the clipboard is dirty
	for (ClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		BaseClientProxy* client = index->second;
		client->setClipboardDirty(id, client != sender);
	}

	// send the new clipboard to the active screen
	m_active->setClipboard(id, &clipboard.m_clipboard);
}

void
Server::onScreensaver(bool activated)
{
	LOG((CLOG_DEBUG "onScreenSaver %s", activated ? "activated" : "deactivated"));

	if (activated) {
		// save current screen and position
		m_activeSaver = m_active;
		m_xSaver      = m_x;
		m_ySaver      = m_y;

		// jump to primary screen
		if (m_active != m_primaryClient) {
			switchScreen(m_primaryClient, 0, 0, true);
		}
	}
	else {
		// jump back to previous screen and position.  we must check
		// that the position is still valid since the screen may have
		// changed resolutions while the screen saver was running.
		if (m_activeSaver != nullptr && m_activeSaver != m_primaryClient) {
			// check position
			BaseClientProxy* screen = m_activeSaver;
			std::int32_t x, y, w, h;
			screen->getShape(x, y, w, h);
			std::int32_t zoneSize = getJumpZoneSize(screen);
			if (m_xSaver < x + zoneSize) {
				m_xSaver = x + zoneSize;
			}
			else if (m_xSaver >= x + w - zoneSize) {
				m_xSaver = x + w - zoneSize - 1;
			}
			if (m_ySaver < y + zoneSize) {
				m_ySaver = y + zoneSize;
			}
			else if (m_ySaver >= y + h - zoneSize) {
				m_ySaver = y + h - zoneSize - 1;
			}

			// jump
			switchScreen(screen, m_xSaver, m_ySaver, false);
		}

		// reset state
		m_activeSaver = nullptr;
	}

	// send message to all clients
	for (ClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		BaseClientProxy* client = index->second;
		client->screensaver(activated);
	}
}

void
Server::onKeyDown(KeyID id, KeyModifierMask mask, KeyButton button,
				const char* screens)
{
	LOG((CLOG_DEBUG1 "onKeyDown id=%d mask=0x%04x button=0x%04x", id, mask, button));
	assert(m_active != nullptr);

	// relay
	if (!m_keyboardBroadcasting && IKeyState::KeyInfo::isDefault(screens)) {
		m_active->keyDown(id, mask, button);
	}
	else {
		if (!screens && m_keyboardBroadcasting) {
			screens = m_keyboardBroadcastingScreens.c_str();
			if (IKeyState::KeyInfo::isDefault(screens)) {
				screens = "*";
			}
		}
		for (ClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
			if (IKeyState::KeyInfo::contains(screens, index->first)) {
				index->second->keyDown(id, mask, button);
			}
		}
	}
}

void
Server::onKeyUp(KeyID id, KeyModifierMask mask, KeyButton button,
				const char* screens)
{
	LOG((CLOG_DEBUG1 "onKeyUp id=%d mask=0x%04x button=0x%04x", id, mask, button));
	assert(m_active != nullptr);

	// relay
	if (!m_keyboardBroadcasting && IKeyState::KeyInfo::isDefault(screens)) {
		m_active->keyUp(id, mask, button);
	}
	else {
		if (!screens && m_keyboardBroadcasting) {
			screens = m_keyboardBroadcastingScreens.c_str();
			if (IKeyState::KeyInfo::isDefault(screens)) {
				screens = "*";
			}
		}
		for (ClientList::const_iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
			if (IKeyState::KeyInfo::contains(screens, index->first)) {
				index->second->keyUp(id, mask, button);
			}
		}
	}
}

void Server::onKeyRepeat(KeyID id, KeyModifierMask mask, std::int32_t count, KeyButton button)
{
	LOG((CLOG_DEBUG1 "onKeyRepeat id=%d mask=0x%04x count=%d button=0x%04x", id, mask, count, button));
	assert(m_active != nullptr);

	// relay
	m_active->keyRepeat(id, mask, count, button);
}

void
Server::onMouseDown(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseDown id=%d", id));
	assert(m_active != nullptr);

	// relay
	m_active->mouseDown(id);

	// reset this variable back to default value true
	m_waitDragInfoThread = true;
}

void
Server::onMouseUp(ButtonID id)
{
	LOG((CLOG_DEBUG1 "onMouseUp id=%d", id));
	assert(m_active != nullptr);

	// relay
	m_active->mouseUp(id);

	if (m_ignoreFileTransfer) {
		m_ignoreFileTransfer = false;
		return;
	}

	if (m_args.m_enableDragDrop) {
		if (!m_screen->isOnScreen()) {
            std::string& file = m_screen->getDraggingFilename();
			if (!file.empty()) {
				sendFileToClient(file.c_str());
			}
		}

		// always clear dragging filename
		m_screen->clearDraggingFilename();
	}
}

bool Server::onMouseMovePrimary(std::int32_t x, std::int32_t y)
{
	LOG((CLOG_DEBUG4 "onMouseMovePrimary %d,%d", x, y));

	// mouse move on primary (server's) screen
	if (m_active != m_primaryClient) {
		// stale event -- we're actually on a secondary screen
		return false;
	}

	// save last delta
	m_xDelta2 = m_xDelta;
	m_yDelta2 = m_yDelta;

	// save current delta
	m_xDelta  = x - m_x;
	m_yDelta  = y - m_y;

	// save position
	m_x       = x;
	m_y       = y;

	// get screen shape
	std::int32_t ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);
	std::int32_t zoneSize = getJumpZoneSize(m_active);

	// clamp position to screen
	std::int32_t xc = x, yc = y;
	if (xc < ax + zoneSize) {
		xc = ax;
	}
	else if (xc >= ax + aw - zoneSize) {
		xc = ax + aw - 1;
	}
	if (yc < ay + zoneSize) {
		yc = ay;
	}
	else if (yc >= ay + ah - zoneSize) {
		yc = ay + ah - 1;
	}

	// see if we should change screens
	// when the cursor is in a corner, there may be a screen either
	// horizontally or vertically.  check both directions.
	EDirection dirh = kNoDirection, dirv = kNoDirection;
	std::int32_t xh = x, yv = y;
	if (x < ax + zoneSize) {
		xh  -= zoneSize;
		dirh = kLeft;
	}
	else if (x >= ax + aw - zoneSize) {
		xh  += zoneSize;
		dirh = kRight;
	}
	if (y < ay + zoneSize) {
		yv  -= zoneSize;
		dirv = kTop;
	}
	else if (y >= ay + ah - zoneSize) {
		yv  += zoneSize;
		dirv = kBottom;
	}
	if (dirh == kNoDirection && dirv == kNoDirection) {
		// still on local screen
		noSwitch(x, y);
		return false;
	}

	// check both horizontally and vertically
	EDirection dirs[] = {dirh, dirv};
	std::int32_t xs[] = {xh, x}, ys[] = {y, yv};
	for (int i = 0; i < 2; ++i) {
		EDirection dir = dirs[i];
		if (dir == kNoDirection) {
			continue;
		}
		x = xs[i], y = ys[i];

		// get jump destination
		BaseClientProxy* newScreen = mapToNeighbor(m_active, dir, x, y);

		// should we switch or not?
		if (isSwitchOkay(newScreen, dir, x, y, xc, yc)) {
			if (m_args.m_enableDragDrop
				&& m_screen->isDraggingStarted()
				&& m_active != newScreen
				&& m_waitDragInfoThread) {
				if (m_sendDragInfoThread == nullptr) {
                    m_sendDragInfoThread = new Thread([this, newScreen]()
                                                      { send_drag_info_thread(newScreen); });
				}

				return false;
			}

			// switch screen
			switchScreen(newScreen, x, y, false);
			m_waitDragInfoThread = true;
			return true;
		}
	}

	return false;
}

void Server::send_drag_info_thread(BaseClientProxy* newScreen)
{
	m_dragFileList.clear();
    std::string& dragFileList = m_screen->getDraggingFilename();
	if (!dragFileList.empty()) {
		DragInformation di;
		di.setFilename(dragFileList);
		m_dragFileList.push_back(di);
	}

#if defined(__APPLE__)
	// on mac it seems that after faking a LMB up, system would signal back
    // to InputLeap a mouse up event, which doesn't happen on windows. as a
    // result, InputLeap would send dragging file to client twice. This variable
	// is used to ignore the first file sending.
	m_ignoreFileTransfer = true;
#endif

	// send drag file info to client if there is any
	if (m_dragFileList.size() > 0) {
		sendDragInfo(newScreen);
		m_dragFileList.clear();
	}
	m_waitDragInfoThread = false;
	m_sendDragInfoThread = nullptr;
}

void
Server::sendDragInfo(BaseClientProxy* newScreen)
{
    std::string infoString;
    std::uint32_t fileCount = DragInformation::setupDragInfo(m_dragFileList, infoString);

	if (fileCount > 0) {
		char* info = nullptr;
		size_t size = infoString.size();
		info = new char[size];
		memcpy(info, infoString.c_str(), size);

		LOG((CLOG_DEBUG2 "sending drag information to client"));
		LOG((CLOG_DEBUG3 "dragging file list: %s", info));
		LOG((CLOG_DEBUG3 "dragging file list string size: %i", size));
		newScreen->sendDragInfo(fileCount, info, size);
	}
}

void Server::onMouseMoveSecondary(std::int32_t dx, std::int32_t dy)
{
	LOG((CLOG_DEBUG2 "onMouseMoveSecondary %+d,%+d", dx, dy));

	// mouse move on secondary (client's) screen
	assert(m_active != nullptr);
	if (m_active == m_primaryClient) {
		// stale event -- we're actually on the primary screen
		return;
	}

	// if doing relative motion on secondary screens and we're locked
	// to the screen (which activates relative moves) then send a
	// relative mouse motion.  when we're doing this we pretend as if
	// the mouse isn't actually moving because we're expecting some
	// program on the secondary screen to warp the mouse on us, so we
	// have no idea where it really is.
	if (m_relativeMoves && isLockedToScreenServer()) {
		LOG((CLOG_DEBUG2 "relative move on %s by %d,%d", getName(m_active).c_str(), dx, dy));
		m_active->mouseRelativeMove(dx, dy);
		return;
	}

	// save old position
	const std::int32_t xOld = m_x;
	const std::int32_t yOld = m_y;

	// save last delta
	m_xDelta2 = m_xDelta;
	m_yDelta2 = m_yDelta;

	// save current delta
	m_xDelta  = dx;
	m_yDelta  = dy;

	// accumulate motion
	m_x      += dx;
	m_y      += dy;

	// get screen shape
	std::int32_t ax, ay, aw, ah;
	m_active->getShape(ax, ay, aw, ah);

	// find direction of neighbor and get the neighbor
	bool jump = true;
	BaseClientProxy* newScreen;
	do {
		// clamp position to screen
		std::int32_t xc = m_x, yc = m_y;
		if (xc < ax) {
			xc = ax;
		}
		else if (xc >= ax + aw) {
			xc = ax + aw - 1;
		}
		if (yc < ay) {
			yc = ay;
		}
		else if (yc >= ay + ah) {
			yc = ay + ah - 1;
		}

		EDirection dir;
		if (m_x < ax) {
			dir = kLeft;
		}
		else if (m_x > ax + aw - 1) {
			dir = kRight;
		}
		else if (m_y < ay) {
			dir = kTop;
		}
		else if (m_y > ay + ah - 1) {
			dir = kBottom;
		}
		else {
			// we haven't left the screen
			newScreen = m_active;
			jump      = false;

			// if waiting and mouse is not on the border we're waiting
			// on then stop waiting.  also if it's not on the border
			// then arm the double tap.
			if (m_switchScreen != nullptr) {
				bool clearWait;
				std::int32_t zoneSize = m_primaryClient->getJumpZoneSize();
				switch (m_switchDir) {
				case kLeft:
					clearWait = (m_x >= ax + zoneSize);
					break;

				case kRight:
					clearWait = (m_x <= ax + aw - 1 - zoneSize);
					break;

				case kTop:
					clearWait = (m_y >= ay + zoneSize);
					break;

				case kBottom:
					clearWait = (m_y <= ay + ah - 1 + zoneSize);
					break;

				default:
					clearWait = false;
					break;
				}
				if (clearWait) {
					// still on local screen
					noSwitch(m_x, m_y);
				}
			}

			// skip rest of block
			break;
		}

		// try to switch screen.  get the neighbor.
		newScreen = mapToNeighbor(m_active, dir, m_x, m_y);

		// see if we should switch
		if (!isSwitchOkay(newScreen, dir, m_x, m_y, xc, yc)) {
			newScreen = m_active;
			jump      = false;
		}
	} while (false);

	if (jump) {
		if (m_sendFileThread != nullptr) {
			StreamChunker::interruptFile();
			m_sendFileThread = nullptr;
		}

		std::int32_t newX = m_x;
		std::int32_t newY = m_y;

		// switch screens
		switchScreen(newScreen, newX, newY, false);
	}
	else {
		// same screen.  clamp mouse to edge.
		m_x = xOld + dx;
		m_y = yOld + dy;
		if (m_x < ax) {
			m_x = ax;
			LOG((CLOG_DEBUG2 "clamp to left of \"%s\"", getName(m_active).c_str()));
		}
		else if (m_x > ax + aw - 1) {
			m_x = ax + aw - 1;
			LOG((CLOG_DEBUG2 "clamp to right of \"%s\"", getName(m_active).c_str()));
		}
		if (m_y < ay) {
			m_y = ay;
			LOG((CLOG_DEBUG2 "clamp to top of \"%s\"", getName(m_active).c_str()));
		}
		else if (m_y > ay + ah - 1) {
			m_y = ay + ah - 1;
			LOG((CLOG_DEBUG2 "clamp to bottom of \"%s\"", getName(m_active).c_str()));
		}

		// warp cursor if it moved.
		if (m_x != xOld || m_y != yOld) {
			LOG((CLOG_DEBUG2 "move on %s to %d,%d", getName(m_active).c_str(), m_x, m_y));
			m_active->mouseMove(m_x, m_y);
		}
	}
}

void Server::onMouseWheel(std::int32_t xDelta, std::int32_t yDelta)
{
	LOG((CLOG_DEBUG1 "onMouseWheel %+d,%+d", xDelta, yDelta));
	assert(m_active != nullptr);

	// relay
	m_active->mouseWheel(xDelta, yDelta);
}

void Server::on_file_chunk_sending(const FileChunk& chunk)
{
	LOG((CLOG_DEBUG1 "sending file chunk"));
	assert(m_active != nullptr);

	// relay
    m_active->fileChunkSending(chunk.chunk_[0], &chunk.chunk_[1], chunk.m_dataSize);
}

void
Server::onFileReceiveCompleted()
{
	if (isReceivedFileSizeValid()) {
        m_writeToDropDirThread = new Thread([this]() { write_to_drop_dir_thread(); });
	}
}

void Server::write_to_drop_dir_thread()
{
	LOG((CLOG_DEBUG "starting write to drop dir thread"));

	while (m_screen->isFakeDraggingStarted()) {
		inputleap::this_thread_sleep(.1f);
	}

	DropHelper::writeToDir(m_screen->getDropTarget(), m_fakeDragFileList,
					m_receivedFileData);
}

bool
Server::addClient(BaseClientProxy* client)
{
    std::string name = getName(client);
	if (m_clients.count(name) != 0) {
		return false;
	}

	// add event handlers
    m_events->add_handler(EventType::SCREEN_SHAPE_CHANGED, client->getEventTarget(),
                          [this, client](const auto& e){ handle_shape_changed(client); });
    m_events->add_handler(EventType::CLIPBOARD_GRABBED, client->getEventTarget(),
                          [this, client](const auto& e){ handle_clipboard_grabbed(e, client); });
    m_events->add_handler(EventType::CLIPBOARD_CHANGED, client->getEventTarget(),
                          [this, client](const auto& e){ handle_clipboard_changed(e, client); });

	// add to list
	m_clientSet.insert(client);
	m_clients.insert(std::make_pair(name, client));

	// initialize client data
	std::int32_t x, y;
	client->getCursorPos(x, y);
	client->setJumpCursorPos(x, y);

	// tell primary client about the active sides
	m_primaryClient->reconfigure(getActivePrimarySides());

	return true;
}

bool
Server::removeClient(BaseClientProxy* client)
{
	// return false if not in list
	ClientSet::iterator i = m_clientSet.find(client);
	if (i == m_clientSet.end()) {
		return false;
	}

	// remove event handlers
    m_events->removeHandler(EventType::SCREEN_SHAPE_CHANGED, client->getEventTarget());
    m_events->removeHandler(EventType::CLIPBOARD_GRABBED, client->getEventTarget());
    m_events->removeHandler(EventType::CLIPBOARD_CHANGED, client->getEventTarget());

	// remove from list
	m_clients.erase(getName(client));
	m_clientSet.erase(i);

	return true;
}

void
Server::closeClient(BaseClientProxy* client, const char* msg)
{
	assert(client != m_primaryClient);
	assert(msg != nullptr);

	// send message to client.  this message should cause the client
	// to disconnect.  we add this client to the closed client list
	// and install a timer to remove the client if it doesn't respond
	// quickly enough.  we also remove the client from the active
	// client list since we're not going to listen to it anymore.
	// note that this method also works on clients that are not in
	// the m_clients list.  adoptClient() may call us with such a
	// client.
	LOG((CLOG_NOTE "disconnecting client \"%s\"", getName(client).c_str()));

	// send message
	// FIXME -- avoid type cast (kinda hard, though)
    (static_cast<ClientProxy*>(client))->close(msg);

	// install timer.  wait timeout seconds for client to close.
	double timeout = 5.0;
    EventQueueTimer* timer = m_events->newOneShotTimer(timeout, nullptr);
    m_events->add_handler(EventType::TIMER, timer,
                          [this, client](const auto& e)
    {
        handle_client_close_timeout(client);
    });

	// move client to closing list
	removeClient(client);
	m_oldClients.insert(std::make_pair(client, timer));

	// if this client is the active screen then we have to
	// jump off of it
	forceLeaveClient(client);
}

void
Server::closeClients(const Config& config)
{
	// collect the clients that are connected but are being dropped
	// from the configuration (or who's canonical name is changing).
	typedef std::set<BaseClientProxy*> RemovedClients;
	RemovedClients removed;
	for (ClientList::iterator index = m_clients.begin();
								index != m_clients.end(); ++index) {
		if (!config.isCanonicalName(index->first)) {
			removed.insert(index->second);
		}
	}

	// don't close the primary client
	removed.erase(m_primaryClient);

	// now close them.  we collect the list then close in two steps
	// because closeClient() modifies the collection we iterate over.
	for (RemovedClients::iterator index = removed.begin();
								index != removed.end(); ++index) {
		closeClient(*index, kMsgCClose);
	}
}

void
Server::removeActiveClient(BaseClientProxy* client)
{
	if (removeClient(client)) {
		forceLeaveClient(client);
        m_events->removeHandler(EventType::CLIENT_PROXY_DISCONNECTED, client);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
            m_events->add_event(EventType::SERVER_DISCONNECTED, this);
		}
	}
}

void
Server::removeOldClient(BaseClientProxy* client)
{
	OldClients::iterator i = m_oldClients.find(client);
	if (i != m_oldClients.end()) {
        m_events->removeHandler(EventType::CLIENT_PROXY_DISCONNECTED, client);
        m_events->removeHandler(EventType::TIMER, i->second);
		m_events->deleteTimer(i->second);
		m_oldClients.erase(i);
		if (m_clients.size() == 1 && m_oldClients.empty()) {
            m_events->add_event(EventType::SERVER_DISCONNECTED, this);
		}
	}
}

void
Server::forceLeaveClient(BaseClientProxy* client)
{
	BaseClientProxy* active =
		(m_activeSaver != nullptr) ? m_activeSaver : m_active;
	if (active == client) {
		// record new position (center of primary screen)
		m_primaryClient->getCursorCenter(m_x, m_y);

		// stop waiting to switch to this client
		if (active == m_switchScreen) {
			stopSwitch();
		}

		// don't notify active screen since it has probably already
		// disconnected.
		LOG((CLOG_INFO "jump from \"%s\" to \"%s\" at %d,%d", getName(active).c_str(), getName(m_primaryClient).c_str(), m_x, m_y));

		// cut over
		m_active = m_primaryClient;

		// enter new screen (unless we already have because of the
		// screen saver)
		if (m_activeSaver == nullptr) {
			m_primaryClient->enter(m_x, m_y, m_seqNum,
								m_primaryClient->getToggleMask(), false);
		}

        Server::SwitchToScreenInfo info{m_active->getName()};
        m_events->add_event(EventType::SERVER_SCREEN_SWITCHED, this,
                            create_event_data<Server::SwitchToScreenInfo>(info));
	}

	// if this screen had the cursor when the screen saver activated
	// then we can't switch back to it when the screen saver
	// deactivates.
	if (m_activeSaver == client) {
		m_activeSaver = nullptr;
	}

	// tell primary client about the active sides
	m_primaryClient->reconfigure(getActivePrimarySides());
}


//
// Server::ClipboardInfo
//

Server::ClipboardInfo::ClipboardInfo() :
	m_clipboard(),
	m_clipboardData(),
	m_clipboardOwner(),
	m_clipboardSeqNum(0)
{
	// do nothing
}

bool
Server::isReceivedFileSizeValid()
{
	return m_expectedFileSize == m_receivedFileData.size();
}

void
Server::sendFileToClient(const char* filename)
{
	if (m_sendFileThread != nullptr) {
		StreamChunker::interruptFile();
	}

    m_sendFileThread = new Thread([this, filename]() { send_file_thread(filename); });
}

void Server::send_file_thread(const char* filename)
{
	try {
		LOG((CLOG_DEBUG "sending file to client, filename=%s", filename));
		StreamChunker::sendFile(filename, m_events, this);
	}
	catch (std::runtime_error &error) {
		LOG((CLOG_ERR "failed sending file chunks, error: %s", error.what()));
	}

	m_sendFileThread = nullptr;
}

void Server::dragInfoReceived(std::uint32_t fileNum, std::string content)
{
	if (!m_args.m_enableDragDrop) {
		LOG((CLOG_DEBUG "drag drop not enabled, ignoring drag info."));
		return;
	}

	DragInformation::parseDragInfo(m_fakeDragFileList, fileNum, content);

	m_screen->startDraggingFiles(m_fakeDragFileList);
}

} // namespace inputleap
