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

#include "platform/XWindowsScreen.h"

#include "platform/XWindowsClipboard.h"
#include "platform/XWindowsEventQueueBuffer.h"
#include "platform/XWindowsKeyState.h"
#include "platform/XWindowsScreenSaver.h"
#include "platform/XWindowsUtil.h"
#include "inputleap/Clipboard.h"
#include "inputleap/KeyMap.h"
#include "inputleap/XScreen.h"
#include "arch/XArch.h"
#include "arch/Arch.h"
#include "base/Log.h"
#include "base/Stopwatch.h"
#include "base/IEventQueue.h"
#include "base/Time.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace inputleap {

static int xi_opcode;

//
// XWindowsScreen
//

// NOTE -- the X display is shared among several objects but is owned
// by the XWindowsScreen.  Xlib is not reentrant so we must ensure
// that no two objects can simultaneously call Xlib with the display.
// this is easy since we only make X11 calls from the main thread.
// we must also ensure that these objects do not use the display in
// their destructors or, if they do, we can tell them not to.  This
// is to handle unexpected disconnection of the X display, when any
// call on the display is invalid.  In that situation we discard the
// display and the X11 event queue buffer, ignore any calls that try
// to use the display, and wait to be destroyed.

XWindowsScreen* XWindowsScreen::s_screen = nullptr;

XWindowsScreen::XWindowsScreen(
        IXWindowsImpl* impl,
		const char* displayName,
		bool isPrimary,
		int mouseScrollDelta,
		IEventQueue* events) :
    PlatformScreen(events),
    m_isPrimary(isPrimary),
    m_mouseScrollDelta(mouseScrollDelta),
    m_x_accumulatedScroll(0),
    m_y_accumulatedScroll(0),
    m_display(nullptr),
    m_root(None),
    m_window(None),
    m_isOnScreen(m_isPrimary),
    m_x(0), m_y(0),
    m_w(0), m_h(0),
    m_xCenter(0), m_yCenter(0),
    m_xCursor(0), m_yCursor(0),
    m_keyState(nullptr),
    m_lastFocus(None),
    m_lastFocusRevert(RevertToNone),
    m_im(nullptr),
    m_ic(nullptr),
    m_lastKeycode(0),
    m_sequenceNumber(0),
    m_screensaver(nullptr),
    m_screensaverNotify(false),
    m_xtestIsXineramaUnaware(true),
    m_preserveFocus(false),
    m_xkb(false),
    m_xi2detected(false),
    m_xrandr(false),
    m_events(events)
{
    m_impl = impl;
    assert(s_screen == nullptr);

	if (mouseScrollDelta==0) m_mouseScrollDelta=120;
	s_screen = this;

    // initializes Xlib support for concurrent threads.
    if (m_impl->XInitThreads() == 0)
        throw std::runtime_error("XInitThreads() returned zero");

	// set the X I/O error handler so we catch the display disconnecting
    m_impl->XSetIOErrorHandler(&XWindowsScreen::ioErrorHandler);

	try {
		m_display     = openDisplay(displayName);
        m_root        = m_impl->do_DefaultRootWindow(m_display);
		saveShape();
		m_window      = openWindow();
        m_screensaver = new XWindowsScreenSaver(m_impl, m_display,
								m_window, getEventTarget(), events);
        m_keyState    = new XWindowsKeyState(m_impl, m_display, m_xkb, events,
                                             m_keyMap);
		LOG((CLOG_DEBUG "screen shape: %d,%d %dx%d %s", m_x, m_y, m_w, m_h, m_xinerama ? "(xinerama)" : ""));
		LOG((CLOG_DEBUG "window is 0x%08x", m_window));
	}
	catch (...) {
        if (m_display != nullptr) {
            m_impl->XCloseDisplay(m_display);
		}
		throw;
    }

	// primary/secondary screen only initialization
	if (m_isPrimary) {
		m_xi2detected = detectXI2();
		if (m_xi2detected) {
			selectXIRawMotion();
		} else
		{
			// start watching for events on other windows
			selectEvents(m_root);
		}

		// prepare to use input methods
		openIM();
	}
	else {
		// become impervious to server grabs
        m_impl->XTestGrabControl(m_display, True);
	}

	// initialize the clipboards
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        m_clipboard[id] = new XWindowsClipboard(m_impl, m_display, m_window, id);
	}

	// install event handlers
	m_events->add_handler(EventType::SYSTEM, m_events->getSystemTarget(),
                          [this](const auto& e){ handle_system_event(e); });

	// install the platform event queue
    m_events->adoptBuffer(new XWindowsEventQueueBuffer(m_impl,
		m_display, m_window, m_events));
}

XWindowsScreen::~XWindowsScreen()
{
    assert(s_screen != nullptr);
    assert(m_display != nullptr);

    m_events->adoptBuffer(nullptr);
    m_events->removeHandler(EventType::SYSTEM, m_events->getSystemTarget());
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
		delete m_clipboard[id];
	}
    delete m_keyState;
	delete m_screensaver;
    m_keyState = nullptr;
    m_screensaver = nullptr;
    if (m_display != nullptr) {
		// FIXME -- is it safe to clean up the IC and IM without a display?
        if (m_ic != nullptr) {
            m_impl->XDestroyIC(m_ic);
		}
        if (m_im != nullptr) {
            m_impl->XCloseIM(m_im);
		}
        m_impl->XDestroyWindow(m_display, m_window);
        m_impl->XCloseDisplay(m_display);
	}
    m_impl->XSetIOErrorHandler(nullptr);

    s_screen = nullptr;
    delete m_impl;
}

void
XWindowsScreen::enable()
{
	if (!m_isPrimary) {
		// get the keyboard control state
		XKeyboardState keyControl;
        m_impl->XGetKeyboardControl(m_display, &keyControl);
		m_autoRepeat = (keyControl.global_auto_repeat == AutoRepeatModeOn);
		m_keyState->setAutoRepeat(keyControl);

		// move hider window under the cursor center
        m_impl->XMoveWindow(m_display, m_window, m_xCenter, m_yCenter);

		// raise and show the window
		// FIXME -- take focus?
        m_impl->XMapRaised(m_display, m_window);

		// warp the mouse to the cursor center
		fakeMouseMove(m_xCenter, m_yCenter);
	}
}

void
XWindowsScreen::disable()
{
	// release input context focus
    if (m_ic != nullptr) {
        m_impl->XUnsetICFocus(m_ic);
	}

	// unmap the hider/grab window.  this also ungrabs the mouse and
	// keyboard if they're grabbed.
    m_impl->XUnmapWindow(m_display, m_window);

	// restore auto-repeat state
	if (!m_isPrimary && m_autoRepeat) {
		//XAutoRepeatOn(m_display);
	}
}

void
XWindowsScreen::enter()
{
	screensaver(false);

	// release input context focus
    if (m_ic != nullptr) {
        m_impl->XUnsetICFocus(m_ic);
	}

	// set the input focus to what it had been when we took it
	if (m_lastFocus != None) {
		// the window may not exist anymore so ignore errors
		XWindowsUtil::ErrorLock lock(m_display);
        m_impl->XSetInputFocus(m_display, m_lastFocus, m_lastFocusRevert, CurrentTime);
	}

	// Force the DPMS to turn screen back on since we don't
	// actually cause physical hardware input to trigger it
	int dummy;
	CARD16 powerlevel;
	BOOL enabled;
    if (m_impl->DPMSQueryExtension(m_display, &dummy, &dummy) &&
        m_impl->DPMSCapable(m_display) &&
        m_impl->DPMSInfo(m_display, &powerlevel, &enabled))
	{
		if (enabled && powerlevel != DPMSModeOn)
            m_impl->DPMSForceLevel(m_display, DPMSModeOn);
	}

	// unmap the hider/grab window.  this also ungrabs the mouse and
	// keyboard if they're grabbed.
    m_impl->XUnmapWindow(m_display, m_window);

/* maybe call this if entering for the screensaver
	// set keyboard focus to root window.  the screensaver should then
	// pick up key events for when the user enters a password to unlock.
	XSetInputFocus(m_display, PointerRoot, PointerRoot, CurrentTime);
*/

	if (!m_isPrimary) {
		// get the keyboard control state
		XKeyboardState keyControl;
        m_impl->XGetKeyboardControl(m_display, &keyControl);
		m_autoRepeat = (keyControl.global_auto_repeat == AutoRepeatModeOn);
		m_keyState->setAutoRepeat(keyControl);

		// turn off auto-repeat.  we do this so fake key press events don't
		// cause the local server to generate their own auto-repeats of
		// those keys.
		//XAutoRepeatOff(m_display);
	}

	// now on screen
	m_isOnScreen = true;
}

bool
XWindowsScreen::leave()
{
	if (!m_isPrimary) {
		// restore the previous keyboard auto-repeat state.  if the user
		// changed the auto-repeat configuration while on the client then
		// that state is lost.  that's because we can't get notified by
		// the X server when the auto-repeat configuration is changed so
		// we can't track the desired configuration.
		if (m_autoRepeat) {
			//XAutoRepeatOn(m_display);
		}

		// move hider window under the cursor center
        m_impl->XMoveWindow(m_display, m_window, m_xCenter, m_yCenter);
	}

	// raise and show the window
    m_impl->XMapRaised(m_display, m_window);

	// grab the mouse and keyboard, if primary and possible
	if (m_isPrimary && !grabMouseAndKeyboard()) {
        m_impl->XUnmapWindow(m_display, m_window);
		return false;
	}

	// save current focus
    m_impl->XGetInputFocus(m_display, &m_lastFocus, &m_lastFocusRevert);

	// take focus
	if (m_isPrimary || !m_preserveFocus) {
        m_impl->XSetInputFocus(m_display, m_window, RevertToPointerRoot, CurrentTime);
	}

	// now warp the mouse.  we warp after showing the window so we're
	// guaranteed to get the mouse leave event and to prevent the
	// keyboard focus from changing under point-to-focus policies.
	if (m_isPrimary) {
		warpCursor(m_xCenter, m_yCenter);
	}
	else {
		fakeMouseMove(m_xCenter, m_yCenter);
	}

	// set input context focus to our window
    if (m_ic != nullptr) {
		XmbResetIC(m_ic);
        m_impl->XSetICFocus(m_ic);
		m_filtered.clear();
	}

	// now off screen
	m_isOnScreen = false;

	return true;
}

bool
XWindowsScreen::setClipboard(ClipboardID id, const IClipboard* clipboard)
{
	// fail if we don't have the requested clipboard
    if (m_clipboard[id] == nullptr) {
		return false;
	}

	// get the actual time.  ICCCM does not allow CurrentTime.
	Time timestamp = XWindowsUtil::getCurrentTime(
								m_display, m_clipboard[id]->getWindow());

    if (clipboard != nullptr) {
		// save clipboard data
		return Clipboard::copy(m_clipboard[id], clipboard, timestamp);
	}
	else {
		// assert clipboard ownership
		if (!m_clipboard[id]->open(timestamp)) {
			return false;
		}
		m_clipboard[id]->empty();
		m_clipboard[id]->close();
		return true;
	}
}

void
XWindowsScreen::checkClipboards()
{
	// do nothing, we're always up to date
}

void
XWindowsScreen::openScreensaver(bool notify)
{
	m_screensaverNotify = notify;
	if (!m_screensaverNotify) {
		m_screensaver->disable();
	}
}

void
XWindowsScreen::closeScreensaver()
{
	if (!m_screensaverNotify) {
		m_screensaver->enable();
	}
}

void
XWindowsScreen::screensaver(bool activate)
{
	if (activate) {
		m_screensaver->activate();
	}
	else {
		m_screensaver->deactivate();
	}
}

void
XWindowsScreen::resetOptions()
{
	m_xtestIsXineramaUnaware = true;
	m_preserveFocus = false;
}

void
XWindowsScreen::setOptions(const OptionsList& options)
{
    for (std::uint32_t i = 0, n = options.size(); i < n; i += 2) {
		if (options[i] == kOptionXTestXineramaUnaware) {
			m_xtestIsXineramaUnaware = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "XTest is Xinerama unaware %s", m_xtestIsXineramaUnaware ? "true" : "false"));
		}
		else if (options[i] == kOptionScreenPreserveFocus) {
			m_preserveFocus = (options[i + 1] != 0);
			LOG((CLOG_DEBUG1 "Preserve Focus = %s", m_preserveFocus ? "true" : "false"));
		}
	}
}

void XWindowsScreen::setSequenceNumber(std::uint32_t seqNum)
{
	m_sequenceNumber = seqNum;
}

bool
XWindowsScreen::isPrimary() const
{
	return m_isPrimary;
}

void*
XWindowsScreen::getEventTarget() const
{
	return const_cast<XWindowsScreen*>(this);
}

bool
XWindowsScreen::getClipboard(ClipboardID id, IClipboard* clipboard) const
{
    assert(clipboard != nullptr);

	// fail if we don't have the requested clipboard
    if (m_clipboard[id] == nullptr) {
		return false;
	}

	// get the actual time.  ICCCM does not allow CurrentTime.
	Time timestamp = XWindowsUtil::getCurrentTime(
								m_display, m_clipboard[id]->getWindow());

	// copy the clipboard
	return Clipboard::copy(clipboard, m_clipboard[id], timestamp);
}

void XWindowsScreen::getShape(std::int32_t& x, std::int32_t& y, std::int32_t& w, std::int32_t& h) const
{
	x = m_x;
	y = m_y;
	w = m_w;
	h = m_h;
}

void XWindowsScreen::getCursorPos(std::int32_t& x, std::int32_t& y) const
{
	Window root, window;
	int mx, my, xWindow, yWindow;
	unsigned int mask;
    if (m_impl->XQueryPointer(m_display, m_root, &root, &window,
								&mx, &my, &xWindow, &yWindow, &mask)) {
		x = mx;
		y = my;
	}
	else {
		x = m_xCenter;
		y = m_yCenter;
	}
}

void XWindowsScreen::reconfigure(std::uint32_t)
{
	// do nothing
}

void XWindowsScreen::warpCursor(std::int32_t x, std::int32_t y)
{
	// warp mouse
	warpCursorNoFlush(x, y);

	// remove all input events before and including warp
	XEvent event;
    while (m_impl->XCheckMaskEvent(m_display, PointerMotionMask |
								ButtonPressMask | ButtonReleaseMask |
								KeyPressMask | KeyReleaseMask |
								KeymapStateMask,
								&event)) {
		// do nothing
	}

	// save position as last position
	m_xCursor = x;
	m_yCursor = y;
}

std::uint32_t XWindowsScreen::registerHotKey(KeyID key, KeyModifierMask mask)
{
	// only allow certain modifiers
	if ((mask & ~(KeyModifierShift | KeyModifierControl |
				  KeyModifierAlt   | KeyModifierSuper)) != 0) {
		LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
		return 0;
	}

	// fail if no keys
	if (key == kKeyNone && mask == 0) {
		return 0;
	}

	// convert to X
	unsigned int modifiers;
	if (!m_keyState->mapModifiersToX(mask, modifiers)) {
		// can't map all modifiers
		LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
		return 0;
	}
	XWindowsKeyState::KeycodeList keycodes;
	m_keyState->mapKeyToKeycodes(key, keycodes);
	if (key != kKeyNone && keycodes.empty()) {
		// can't map key
		LOG((CLOG_DEBUG "could not map hotkey id=%04x mask=%04x", key, mask));
		return 0;
	}

	// choose hotkey id
    std::uint32_t id;
	if (!m_oldHotKeyIDs.empty()) {
		id = m_oldHotKeyIDs.back();
		m_oldHotKeyIDs.pop_back();
	}
	else {
		id = m_hotKeys.size() + 1;
	}
	HotKeyList& hotKeys = m_hotKeys[id];

	// all modifier hotkey must be treated specially.  for each modifier
	// we need to grab the modifier key in combination with all the other
	// requested modifiers.
	bool err = false;
	{
		XWindowsUtil::ErrorLock lock(m_display, &err);
		if (key == kKeyNone) {
			static const KeyModifierMask s_hotKeyModifiers[] = {
				KeyModifierShift,
				KeyModifierControl,
				KeyModifierAlt,
				KeyModifierMeta,
				KeyModifierSuper
			};

            XModifierKeymap* modKeymap = m_impl->XGetModifierMapping(m_display);
			for (size_t j = 0; j < sizeof(s_hotKeyModifiers) /
									sizeof(s_hotKeyModifiers[0]) && !err; ++j) {
				// skip modifier if not in mask
				if ((mask & s_hotKeyModifiers[j]) == 0) {
					continue;
				}

				// skip with error if we can't map remaining modifiers
				unsigned int modifiers2;
				KeyModifierMask mask2 = (mask & ~s_hotKeyModifiers[j]);
				if (!m_keyState->mapModifiersToX(mask2, modifiers2)) {
					err = true;
					continue;
				}

				// compute modifier index for modifier.  there should be
				// exactly one X modifier missing
				int index;
				switch (modifiers ^ modifiers2) {
				case ShiftMask:
					index = ShiftMapIndex;
					break;

				case LockMask:
					index = LockMapIndex;
					break;

				case ControlMask:
					index = ControlMapIndex;
					break;

				case Mod1Mask:
					index = Mod1MapIndex;
					break;

				case Mod2Mask:
					index = Mod2MapIndex;
					break;

				case Mod3Mask:
					index = Mod3MapIndex;
					break;

				case Mod4Mask:
					index = Mod4MapIndex;
					break;

				case Mod5Mask:
					index = Mod5MapIndex;
					break;

				default:
					err = true;
					continue;
				}

				// grab each key for the modifier
				const KeyCode* modifiermap =
					modKeymap->modifiermap + index * modKeymap->max_keypermod;
				for (int k = 0; k < modKeymap->max_keypermod && !err; ++k) {
					KeyCode code = modifiermap[k];
					if (modifiermap[k] != 0) {
                        m_impl->XGrabKey(m_display, code, modifiers2, m_root,
									False, GrabModeAsync, GrabModeAsync);
						if (!err) {
							hotKeys.push_back(std::make_pair(code, modifiers2));
							m_hotKeyToIDMap[HotKeyItem(code, modifiers2)] = id;
						}
					}
				}
			}
            m_impl->XFreeModifiermap(modKeymap);
		}

		// a non-modifier key must be insensitive to CapsLock, NumLock and
		// ScrollLock, so we have to grab the key with every combination of
		// those.
		else {
			// collect available toggle modifiers
			unsigned int modifier;
			unsigned int toggleModifiers[3];
			size_t numToggleModifiers = 0;
			if (m_keyState->mapModifiersToX(KeyModifierCapsLock, modifier)) {
				toggleModifiers[numToggleModifiers++] = modifier;
			}
			if (m_keyState->mapModifiersToX(KeyModifierNumLock, modifier)) {
				toggleModifiers[numToggleModifiers++] = modifier;
			}
			if (m_keyState->mapModifiersToX(KeyModifierScrollLock, modifier)) {
				toggleModifiers[numToggleModifiers++] = modifier;
			}


			for (XWindowsKeyState::KeycodeList::iterator j = keycodes.begin();
									j != keycodes.end() && !err; ++j) {
				for (size_t i = 0; i < (1u << numToggleModifiers); ++i) {
					// add toggle modifiers for index i
					unsigned int tmpModifiers = modifiers;
					if ((i & 1) != 0) {
						tmpModifiers |= toggleModifiers[0];
					}
					if ((i & 2) != 0) {
						tmpModifiers |= toggleModifiers[1];
					}
					if ((i & 4) != 0) {
						tmpModifiers |= toggleModifiers[2];
					}

					// add grab
                    m_impl->XGrabKey(m_display, *j, tmpModifiers, m_root,
										False, GrabModeAsync, GrabModeAsync);
					if (!err) {
						hotKeys.push_back(std::make_pair(*j, tmpModifiers));
						m_hotKeyToIDMap[HotKeyItem(*j, tmpModifiers)] = id;
					}
				}
			}
		}
	}

	if (err) {
		// if any failed then unregister any we did get
		for (HotKeyList::iterator j = hotKeys.begin();
								j != hotKeys.end(); ++j) {
            m_impl->XUngrabKey(m_display, j->first, j->second, m_root);
			m_hotKeyToIDMap.erase(HotKeyItem(j->first, j->second));
		}

		m_oldHotKeyIDs.push_back(id);
		m_hotKeys.erase(id);
		LOG((CLOG_WARN "failed to register hotkey %s (id=%04x mask=%04x)", inputleap::KeyMap::formatKey(key, mask).c_str(), key, mask));
		return 0;
	}

	LOG((CLOG_DEBUG "registered hotkey %s (id=%04x mask=%04x) as id=%d", inputleap::KeyMap::formatKey(key, mask).c_str(), key, mask, id));
	return id;
}

void XWindowsScreen::unregisterHotKey(std::uint32_t id)
{
	// look up hotkey
	HotKeyMap::iterator i = m_hotKeys.find(id);
	if (i == m_hotKeys.end()) {
		return;
	}

	// unregister with OS
	bool err = false;
	{
		XWindowsUtil::ErrorLock lock(m_display, &err);
		HotKeyList& hotKeys = i->second;
		for (HotKeyList::iterator j = hotKeys.begin();
								j != hotKeys.end(); ++j) {
            m_impl->XUngrabKey(m_display, j->first, j->second, m_root);
			m_hotKeyToIDMap.erase(HotKeyItem(j->first, j->second));
		}
	}
	if (err) {
		LOG((CLOG_WARN "failed to unregister hotkey id=%d", id));
	}
	else {
		LOG((CLOG_DEBUG "unregistered hotkey id=%d", id));
	}

	// discard hot key from map and record old id for reuse
	m_hotKeys.erase(i);
	m_oldHotKeyIDs.push_back(id);
}

void
XWindowsScreen::fakeInputBegin()
{
	// FIXME -- not implemented
}

void
XWindowsScreen::fakeInputEnd()
{
	// FIXME -- not implemented
}

std::int32_t XWindowsScreen::getJumpZoneSize() const
{
	return 1;
}

bool XWindowsScreen::isAnyMouseButtonDown(std::uint32_t& buttonID) const
{
    (void) buttonID;

	// query the pointer to get the button state
	Window root, window;
	int xRoot, yRoot, xWindow, yWindow;
	unsigned int state;
    if (m_impl->XQueryPointer(m_display, m_root, &root, &window,
								&xRoot, &yRoot, &xWindow, &yWindow, &state)) {
		return ((state & (Button1Mask | Button2Mask | Button3Mask |
							Button4Mask | Button5Mask)) != 0);
	}

	return false;
}

void XWindowsScreen::getCursorCenter(std::int32_t& x, std::int32_t& y) const
{
	x = m_xCenter;
	y = m_yCenter;
}

void
XWindowsScreen::fakeMouseButton(ButtonID button, bool press)
{
	const unsigned int xButton = mapButtonToX(button);
	if (xButton > 0 && xButton < 11) {
        m_impl->XTestFakeButtonEvent(m_display, xButton,
							press ? True : False, CurrentTime);
        m_impl->XFlush(m_display);
	}
}

void XWindowsScreen::fakeMouseMove(std::int32_t x, std::int32_t y)
{
	if (m_xinerama && m_xtestIsXineramaUnaware) {
        m_impl->XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);
	}
	else {
		XTestFakeMotionEvent(m_display, DefaultScreen(m_display),
							x, y, CurrentTime);
	}
    m_impl->XFlush(m_display);
}

void XWindowsScreen::fakeMouseRelativeMove(std::int32_t dx, std::int32_t dy) const
{
	// FIXME -- ignore xinerama for now
	if (false && m_xinerama && m_xtestIsXineramaUnaware) {
//		m_impl->XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);
	}
	else {
        m_impl->XTestFakeRelativeMotionEvent(m_display, dx, dy, CurrentTime);
	}
    m_impl->XFlush(m_display);
}

void XWindowsScreen::fakeMouseWheel(std::int32_t xDelta, std::int32_t yDelta) const
{
    int numEvents;

    if ((!xDelta && !yDelta) || (xDelta && yDelta)) {
        // Invalid scrolling inputs
        return;
    }

    // 4,  5,    6,    7
    // up, down, left, right
    unsigned int xButton;

    if (yDelta) { // vertical scroll
        numEvents = y_accumulateMouseScroll(yDelta);
        if (numEvents >= 0) {
            xButton = 4; // up
        }
        else {
            xButton = 5; // down
        }
    }
    else { // horizontal scroll
        numEvents = x_accumulateMouseScroll(xDelta);
        if (numEvents >= 0) {
            xButton = 7; // right
        }
        else {
            xButton = 6; // left
        }
    }

    numEvents = std::abs(numEvents);

	// send as many clicks as necessary
    for (; numEvents > 0; numEvents--) {
        m_impl->XTestFakeButtonEvent(m_display, xButton, True, CurrentTime);
        m_impl->XTestFakeButtonEvent(m_display, xButton, False, CurrentTime);
	}

    m_impl->XFlush(m_display);
}

Display*
XWindowsScreen::openDisplay(const char* displayName)
{
	// get the DISPLAY
    if (displayName == nullptr) {
		displayName = std::getenv("DISPLAY");
        if (displayName == nullptr) {
			displayName = ":0.0";
		}
	}

	// open the display
	LOG((CLOG_DEBUG "XOpenDisplay(\"%s\")", displayName));
    Display* display = m_impl->XOpenDisplay(displayName);
    if (display == nullptr) {
		throw XScreenUnavailable(60.0);
	}

	// verify the availability of the XTest extension
	if (!m_isPrimary) {
		int majorOpcode, firstEvent, firstError;
        if (!m_impl->XQueryExtension(display, XTestExtensionName,
							&majorOpcode, &firstEvent, &firstError)) {
			LOG((CLOG_ERR "XTEST extension not available"));
            m_impl->XCloseDisplay(display);
			throw XScreenOpenFailure();
		}
	}

	{
		m_xkb = false;
		int major = XkbMajorVersion, minor = XkbMinorVersion;
        if (m_impl->XkbLibraryVersion(&major, &minor)) {
			int opcode, firstError;
            if (m_impl->XkbQueryExtension(display, &opcode, &m_xkbEventBase,
								&firstError, &major, &minor)) {
				m_xkb = true;
                m_impl->XkbSelectEvents(display, XkbUseCoreKbd,
								XkbMapNotifyMask, XkbMapNotifyMask);
                m_impl->XkbSelectEventDetails(display, XkbUseCoreKbd,
								XkbStateNotifyMask,
								XkbGroupStateMask, XkbGroupStateMask);
			}
		}
	}

	// query for XRandR extension
	int dummyError;
    m_xrandr = m_impl->XRRQueryExtension(display, &m_xrandrEventBase, &dummyError);
	if (m_xrandr) {
		// enable XRRScreenChangeNotifyEvent
        m_impl->XRRSelectInput(display, DefaultRootWindow(display), RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask);
	}

	return display;
}

void
XWindowsScreen::saveShape()
{
	// get shape of default screen
	m_x = 0;
	m_y = 0;

	m_w = WidthOfScreen(DefaultScreenOfDisplay(m_display));
	m_h = HeightOfScreen(DefaultScreenOfDisplay(m_display));

	// get center of default screen
	m_xCenter = m_x + (m_w >> 1);
	m_yCenter = m_y + (m_h >> 1);

	// check if xinerama is enabled and there is more than one screen.
	// get center of first Xinerama screen.  Xinerama appears to have
	// a bug when XWarpPointer() is used in combination with
	// XGrabPointer().  in that case, the warp is successful but the
	// next pointer motion warps the pointer again, apparently to
	// constrain it to some unknown region, possibly the region from
	// 0,0 to Wm,Hm where Wm (Hm) is the minimum width (height) over
	// all physical screens.  this warp only seems to happen if the
	// pointer wasn't in that region before the XWarpPointer().  the
    // second (unexpected) warp causes InputLeap to think the pointer
	// has been moved when it hasn't.  to work around the problem,
	// we warp the pointer to the center of the first physical
	// screen instead of the logical screen.
	m_xinerama = false;
	int eventBase, errorBase;
    if (m_impl->XineramaQueryExtension(m_display, &eventBase, &errorBase) &&
        m_impl->XineramaIsActive(m_display)) {
		int numScreens;
		XineramaScreenInfo* screens;
        screens = reinterpret_cast<XineramaScreenInfo*>(
            XineramaQueryScreens(m_display, &numScreens));

        if (screens != nullptr) {
			if (numScreens > 1) {
				m_xinerama = true;
				m_xCenter  = screens[0].x_org + (screens[0].width  >> 1);
				m_yCenter  = screens[0].y_org + (screens[0].height >> 1);
			}
			XFree(screens);
		}
	}
}

Window
XWindowsScreen::openWindow() const
{
	// default window attributes.  we don't want the window manager
	// messing with our window and we don't want the cursor to be
	// visible inside the window.
	XSetWindowAttributes attr;
	attr.do_not_propagate_mask = 0;
	attr.override_redirect     = True;
	attr.cursor                = createBlankCursor();

	// adjust attributes and get size and shape
	std::int32_t x, y, w, h;
	if (m_isPrimary) {
		// grab window attributes.  this window is used to capture user
		// input when the user is focused on another client.  it covers
		// the whole screen.
		attr.event_mask = PointerMotionMask |
							 ButtonPressMask | ButtonReleaseMask |
							 KeyPressMask | KeyReleaseMask |
							 KeymapStateMask | PropertyChangeMask;
		x = m_x;
		y = m_y;
		w = m_w;
		h = m_h;
	}
	else {
		// cursor hider window attributes.  this window is used to hide the
		// cursor when it's not on the screen.  the window is hidden as soon
		// as the cursor enters the screen or the display's real mouse is
		// moved.  we'll reposition the window as necessary so its
		// position here doesn't matter.  it only needs to be 1x1 because
		// it only needs to contain the cursor's hotspot.
		attr.event_mask = LeaveWindowMask;
		x = 0;
		y = 0;
		w = 1;
		h = 1;
	}

	// create and return the window
    Window window = m_impl->XCreateWindow(m_display, m_root, x, y, w, h, 0, 0,
                            InputOnly, reinterpret_cast<Visual*>(CopyFromParent),
							CWDontPropagate | CWEventMask |
							CWOverrideRedirect | CWCursor,
							&attr);
	if (window == None) {
		throw XScreenOpenFailure();
	}
	return window;
}

void
XWindowsScreen::openIM()
{
	// open the input methods
    XIM im = m_impl->XOpenIM(m_display, nullptr, nullptr, nullptr);
    if (im == nullptr) {
		LOG((CLOG_INFO "no support for IM"));
		return;
	}

    // find the appropriate style.  InputLeap supports XIMPreeditNothing
	// only at the moment.
	XIMStyles* styles;
    if (m_impl->XGetIMValues(im, XNQueryInputStyle, &styles) != nullptr ||
        styles == nullptr) {
		LOG((CLOG_WARN "cannot get IM styles"));
        m_impl->XCloseIM(im);
		return;
	}
	XIMStyle style = 0;
	for (unsigned short i = 0; i < styles->count_styles; ++i) {
		style = styles->supported_styles[i];
		if ((style & XIMPreeditNothing) != 0) {
			if ((style & (XIMStatusNothing | XIMStatusNone)) != 0) {
				break;
			}
		}
	}
	XFree(styles);
	if (style == 0) {
		LOG((CLOG_INFO "no supported IM styles"));
        m_impl->XCloseIM(im);
		return;
	}

	// create an input context for the style and tell it about our window
    XIC ic = m_impl->XCreateIC(im, XNInputStyle, style, XNClientWindow, m_window);
    if (ic == nullptr) {
		LOG((CLOG_WARN "cannot create IC"));
        m_impl->XCloseIM(im);
		return;
	}

	// find out the events we must select for and do so
	unsigned long mask;
    if (m_impl->XGetICValues(ic, XNFilterEvents, &mask) != nullptr) {
		LOG((CLOG_WARN "cannot get IC filter events"));
        m_impl->XDestroyIC(ic);
        m_impl->XCloseIM(im);
		return;
	}

	// we have IM
	m_im          = im;
	m_ic          = ic;
	m_lastKeycode = 0;

	// select events on our window that IM requires
	XWindowAttributes attr;
    m_impl->XGetWindowAttributes(m_display, m_window, &attr);
    m_impl->XSelectInput(m_display, m_window, attr.your_event_mask | mask);
}

void XWindowsScreen::sendEvent(EventType type, EventDataBase* data)
{
    m_events->add_event(type, getEventTarget(), data);
}

void XWindowsScreen::sendClipboardEvent(EventType type, ClipboardID id)
{
    ClipboardInfo info;
    info.m_id = id;
    info.m_sequenceNumber = m_sequenceNumber;
    sendEvent(type, create_event_data<ClipboardInfo>(info));
}

IKeyState*
XWindowsScreen::getKeyState() const
{
	return m_keyState;
}

Bool
XWindowsScreen::findKeyEvent(Display*, XEvent* xevent, XPointer arg)
{
	KeyEventFilter* filter = reinterpret_cast<KeyEventFilter*>(arg);
	return (xevent->type         == filter->m_event &&
			xevent->xkey.window  == filter->m_window &&
			xevent->xkey.time    == filter->m_time &&
			xevent->xkey.keycode == filter->m_keycode) ? True : False;
}

void
XWindowsScreen::handle_system_event(const Event& event)
{
    XEvent* xevent = event.get_data_as<XEvent*>();
    assert(xevent != nullptr);

	// update key state
	bool isRepeat = false;
	if (m_isPrimary) {
		if (xevent->type == KeyRelease) {
			// check if this is a key repeat by getting the next
			// KeyPress event that has the same key and time as
			// this release event, if any.  first prepare the
			// filter info.
			KeyEventFilter filter;
			filter.m_event   = KeyPress;
			filter.m_window  = xevent->xkey.window;
			filter.m_time    = xevent->xkey.time;
			filter.m_keycode = xevent->xkey.keycode;
			XEvent xevent2;
            isRepeat = (m_impl->XCheckIfEvent(m_display, &xevent2,
							&XWindowsScreen::findKeyEvent,
                            reinterpret_cast<XPointer>(&filter)) == True);
		}

		if (xevent->type == KeyPress || xevent->type == KeyRelease) {
			if (xevent->xkey.window == m_root) {
				// this is a hot key
				onHotKey(xevent->xkey, isRepeat);
				return;
			}
			else if (!m_isOnScreen) {
				// this might be a hot key
				if (onHotKey(xevent->xkey, isRepeat)) {
					return;
				}
			}

			bool down             = (isRepeat || xevent->type == KeyPress);
			KeyModifierMask state =
				m_keyState->mapModifiersFromX(xevent->xkey.state);
			m_keyState->onKey(xevent->xkey.keycode, down, state);
		}
	}

	// let input methods try to handle event first
    if (m_ic != nullptr) {
		// XFilterEvent() may eat the event and generate a new KeyPress
		// event with a keycode of 0 because there isn't an actual key
		// associated with the keysym.  but the KeyRelease may pass
		// through XFilterEvent() and keep its keycode.  this means
		// there's a mismatch between KeyPress and KeyRelease keycodes.
		// since we use the keycode on the client to detect when a key
		// is released this won't do.  so we remember the keycode on
		// the most recent KeyPress (and clear it on a matching
		// KeyRelease) so we have a keycode for a synthesized KeyPress.
		if (xevent->type == KeyPress && xevent->xkey.keycode != 0) {
			m_lastKeycode = xevent->xkey.keycode;
		}
		else if (xevent->type == KeyRelease &&
			xevent->xkey.keycode == m_lastKeycode) {
			m_lastKeycode = 0;
		}

		// now filter the event
        if (m_impl->XFilterEvent(xevent, DefaultRootWindow(m_display))) {
			if (xevent->type == KeyPress) {
				// add filtered presses to the filtered list
				m_filtered.insert(m_lastKeycode);
			}
			return;
		}

		// discard matching key releases for key presses that were
		// filtered and remove them from our filtered list.
		else if (xevent->type == KeyRelease &&
			m_filtered.count(xevent->xkey.keycode) > 0) {
			m_filtered.erase(xevent->xkey.keycode);
			return;
		}
	}

	// let screen saver have a go
	if (m_screensaver->handleXEvent(xevent)) {
		// screen saver handled it
		return;
	}

	if (m_xi2detected) {
		// Process RawMotion
        XGenericEventCookie *cookie = static_cast<XGenericEventCookie*>(&xevent->xcookie);
            if (m_impl->XGetEventData(m_display, cookie) &&
				cookie->type == GenericEvent &&
				cookie->extension == xi_opcode) {
			if (cookie->evtype == XI_RawMotion) {
				// Get current pointer's position
				XMotionEvent xmotion;
				xmotion.type = MotionNotify;
				xmotion.send_event = False; // Raw motion
				xmotion.display = m_display;
				xmotion.window = m_window;
				/* xmotion's time, state and is_hint are not used */
				unsigned int msk;
                    xmotion.same_screen = m_impl->XQueryPointer(
						m_display, m_root, &xmotion.root, &xmotion.subwindow,
						&xmotion.x_root,
						&xmotion.y_root,
						&xmotion.x,
						&xmotion.y,
						&msk);
					onMouseMove(xmotion);
                    m_impl->XFreeEventData(m_display, cookie);
					return;
			}
                m_impl->XFreeEventData(m_display, cookie);
		}
	}

	// handle the event ourself
	switch (xevent->type) {
	case CreateNotify:
		if (m_isPrimary && !m_xi2detected) {
			// select events on new window
			selectEvents(xevent->xcreatewindow.window);
		}
		break;

	case MappingNotify:
		refreshKeyboard(xevent);
		break;

	case LeaveNotify:
		if (!m_isPrimary) {
			// mouse moved out of hider window somehow.  hide the window.
            m_impl->XUnmapWindow(m_display, m_window);
		}
		break;

	case SelectionClear:
		{
			// we just lost the selection.  that means someone else
			// grabbed the selection so this screen is now the
			// selection owner.  report that to the receiver.
			ClipboardID id = getClipboardID(xevent->xselectionclear.selection);
			if (id != kClipboardEnd) {
				m_clipboard[id]->lost(xevent->xselectionclear.time);
                sendClipboardEvent(EventType::CLIPBOARD_GRABBED, id);
				return;
			}
		}
		break;

	case SelectionNotify:
		// notification of selection transferred.  we shouldn't
		// get this here because we handle them in the selection
		// retrieval methods.  we'll just delete the property
		// with the data (satisfying the usual ICCCM protocol).
		if (xevent->xselection.property != None) {
            m_impl->XDeleteProperty(m_display,
								xevent->xselection.requestor,
								xevent->xselection.property);
		}
		break;

	case SelectionRequest:
		{
			// somebody is asking for clipboard data
			ClipboardID id = getClipboardID(
								xevent->xselectionrequest.selection);
			if (id != kClipboardEnd) {
				m_clipboard[id]->addRequest(
								xevent->xselectionrequest.owner,
								xevent->xselectionrequest.requestor,
								xevent->xselectionrequest.target,
								xevent->xselectionrequest.time,
								xevent->xselectionrequest.property);
				return;
			}
		}
		break;

	case PropertyNotify:
		// property delete may be part of a selection conversion
		if (xevent->xproperty.state == PropertyDelete) {
			processClipboardRequest(xevent->xproperty.window,
								xevent->xproperty.time,
								xevent->xproperty.atom);
		}
		break;

	case DestroyNotify:
		// looks like one of the windows that requested a clipboard
		// transfer has gone bye-bye.
		destroyClipboardRequest(xevent->xdestroywindow.window);
		break;

	case KeyPress:
		if (m_isPrimary) {
			onKeyPress(xevent->xkey);
		}
		return;

	case KeyRelease:
		if (m_isPrimary) {
			onKeyRelease(xevent->xkey, isRepeat);
		}
		return;

	case ButtonPress:
		if (m_isPrimary) {
			onMousePress(xevent->xbutton);
		}
		return;

	case ButtonRelease:
		if (m_isPrimary) {
			onMouseRelease(xevent->xbutton);
		}
		return;

	case MotionNotify:
		if (m_isPrimary) {
			onMouseMove(xevent->xmotion);
		}
		return;

	default:
		if (m_xkb && xevent->type == m_xkbEventBase) {
			XkbEvent* xkbEvent = reinterpret_cast<XkbEvent*>(xevent);
			switch (xkbEvent->any.xkb_type) {
			case XkbMapNotify:
				refreshKeyboard(xevent);
				return;

			case XkbStateNotify:
				LOG((CLOG_INFO "group change: %d", xkbEvent->state.group));
                m_keyState->setActiveGroup(static_cast<std::int32_t>(xkbEvent->state.group));
				return;
            default:
                break;
			}
		}

		if (m_xrandr) {
			if (xevent->type == m_xrandrEventBase + RRScreenChangeNotify ||
			    (xevent->type == m_xrandrEventBase + RRNotify &&
			     reinterpret_cast<XRRNotifyEvent *>(xevent)->subtype == RRNotify_CrtcChange)) {
				LOG((CLOG_INFO "XRRScreenChangeNotifyEvent or RRNotify_CrtcChange received"));

				// we're required to call back into XLib so XLib can update its internal state
				XRRUpdateConfiguration(xevent);

				// requery/recalculate the screen shape
				saveShape();

				// we need to resize m_window, otherwise we'll get a weird problem where moving
				// off the server onto the client causes the pointer to warp to the
				// center of the server (so you can't move the pointer off the server)
				if (m_isPrimary) {
                    m_impl->XMoveWindow(m_display, m_window, m_x, m_y);
                    m_impl->XResizeWindow(m_display, m_window, m_w, m_h);
				}

                sendEvent(EventType::SCREEN_SHAPE_CHANGED);
			}
		}

		break;
	}
}

void
XWindowsScreen::onKeyPress(XKeyEvent& xkey)
{
	LOG((CLOG_DEBUG1 "event: KeyPress code=%d, state=0x%04x", xkey.keycode, xkey.state));
	const KeyModifierMask mask = m_keyState->mapModifiersFromX(xkey.state);
	KeyID key                  = mapKeyFromX(&xkey);
	if (key != kKeyNone) {
		// check for ctrl+alt+del emulation
		if ((key == kKeyPause || key == kKeyBreak) &&
			(mask & (KeyModifierControl | KeyModifierAlt)) ==
					(KeyModifierControl | KeyModifierAlt)) {
			// pretend it's ctrl+alt+del
			LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
			key = kKeyDelete;
		}

		// get which button.  see call to XFilterEvent() in onEvent()
		// for more info.
		bool isFake = false;
		KeyButton keycode = static_cast<KeyButton>(xkey.keycode);
		if (keycode == 0) {
			isFake  = true;
			keycode = static_cast<KeyButton>(m_lastKeycode);
			if (keycode == 0) {
				// no keycode
				LOG((CLOG_DEBUG1 "event: KeyPress no keycode"));
				return;
			}
		}

		// handle key
		m_keyState->sendKeyEvent(getEventTarget(),
							true, false, key, mask, 1, keycode);

		// do fake release if this is a fake press
		if (isFake) {
			m_keyState->sendKeyEvent(getEventTarget(),
							false, false, key, mask, 1, keycode);
		}
	}
    else {
		LOG((CLOG_DEBUG1 "can't map keycode to key id"));
    }
}

void
XWindowsScreen::onKeyRelease(XKeyEvent& xkey, bool isRepeat)
{
	const KeyModifierMask mask = m_keyState->mapModifiersFromX(xkey.state);
	KeyID key                  = mapKeyFromX(&xkey);
	if (key != kKeyNone) {
		// check for ctrl+alt+del emulation
		if ((key == kKeyPause || key == kKeyBreak) &&
			(mask & (KeyModifierControl | KeyModifierAlt)) ==
					(KeyModifierControl | KeyModifierAlt)) {
			// pretend it's ctrl+alt+del and ignore autorepeat
			LOG((CLOG_DEBUG "emulate ctrl+alt+del"));
			key      = kKeyDelete;
			isRepeat = false;
		}

		KeyButton keycode = static_cast<KeyButton>(xkey.keycode);
		if (!isRepeat) {
			// no press event follows so it's a plain release
			LOG((CLOG_DEBUG1 "event: KeyRelease code=%d, state=0x%04x", keycode, xkey.state));
			m_keyState->sendKeyEvent(getEventTarget(),
							false, false, key, mask, 1, keycode);
		}
		else {
			// found a press event following so it's a repeat.
			// we could attempt to count the already queued
			// repeats but we'll just send a repeat of 1.
			// note that we discard the press event.
			LOG((CLOG_DEBUG1 "event: repeat code=%d, state=0x%04x", keycode, xkey.state));
			m_keyState->sendKeyEvent(getEventTarget(),
							false, true, key, mask, 1, keycode);
		}
	}
}

bool
XWindowsScreen::onHotKey(XKeyEvent& xkey, bool isRepeat)
{
	// find the hot key id
	HotKeyToIDMap::const_iterator i =
		m_hotKeyToIDMap.find(HotKeyItem(xkey.keycode, xkey.state));
	if (i == m_hotKeyToIDMap.end()) {
		return false;
	}

	// find what kind of event
    EventType type;
	if (xkey.type == KeyPress) {
        type = EventType::PRIMARY_SCREEN_HOTKEY_DOWN;
	}
	else if (xkey.type == KeyRelease) {
        type = EventType::PRIMARY_SCREEN_HOTKEY_UP;
	}
	else {
		return false;
	}

	// generate event (ignore key repeats)
	if (!isRepeat) {
        m_events->add_event(type, getEventTarget(),
                            create_event_data<HotKeyInfo>(HotKeyInfo{i->second}));
	}
	return true;
}

void
XWindowsScreen::onMousePress(const XButtonEvent& xbutton)
{
	LOG((CLOG_DEBUG1 "event: ButtonPress button=%d", xbutton.button));
	ButtonID button      = mapButtonFromX(&xbutton);
	KeyModifierMask mask = m_keyState->mapModifiersFromX(xbutton.state);
	if (button != kButtonNone) {
        sendEvent(EventType::PRIMARY_SCREEN_BUTTON_DOWN,
                  create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
	}
}

void
XWindowsScreen::onMouseRelease(const XButtonEvent& xbutton)
{
	LOG((CLOG_DEBUG1 "event: ButtonRelease button=%d", xbutton.button));
	ButtonID button      = mapButtonFromX(&xbutton);
	KeyModifierMask mask = m_keyState->mapModifiersFromX(xbutton.state);
	if (button != kButtonNone) {
        sendEvent(EventType::PRIMARY_SCREEN_BUTTON_UP,
                  create_event_data<ButtonInfo>(ButtonInfo{button, mask}));
	}
	else if (xbutton.button == 4) {
		// wheel forward (away from user)
        sendEvent(EventType::PRIMARY_SCREEN_WHEEL,
                  create_event_data<WheelInfo>(WheelInfo{0, 120}));
	}
	else if (xbutton.button == 5) {
		// wheel backward (toward user)
        sendEvent(EventType::PRIMARY_SCREEN_WHEEL,
                  create_event_data<WheelInfo>(WheelInfo{0, -120}));
	}
	else if (xbutton.button == 6) {
		// wheel left
        sendEvent(EventType::PRIMARY_SCREEN_WHEEL,
                  create_event_data<WheelInfo>(WheelInfo{-120, 0}));
	}
	else if (xbutton.button == 7) {
		// wheel right
        sendEvent(EventType::PRIMARY_SCREEN_WHEEL,
                  create_event_data<WheelInfo>(WheelInfo{120, 0}));
	}
}

void
XWindowsScreen::onMouseMove(const XMotionEvent& xmotion)
{
	LOG((CLOG_DEBUG2 "event: MotionNotify %d,%d", xmotion.x_root, xmotion.y_root));

	// compute motion delta (relative to the last known
	// mouse position)
	std::int32_t x = xmotion.x_root - m_xCursor;
	std::int32_t y = xmotion.y_root - m_yCursor;

	// save position to compute delta of next motion
	m_xCursor = xmotion.x_root;
	m_yCursor = xmotion.y_root;

	if (xmotion.send_event) {
		// we warped the mouse.  discard events until we
		// find the matching sent event.  see
		// warpCursorNoFlush() for where the events are
		// sent.  we discard the matching sent event and
		// can be sure we've skipped the warp event.
		XEvent xevent;
		char cntr = 0;
		do {
            m_impl->XMaskEvent(m_display, PointerMotionMask, &xevent);
			if (cntr++ > 10) {
				LOG((CLOG_WARN "too many discarded events! %d", cntr));
				break;
			}
		} while (!xevent.xany.send_event);
		cntr = 0;
	}
	else if (m_isOnScreen) {
		// motion on primary screen
        sendEvent(EventType::PRIMARY_SCREEN_MOTION_ON_PRIMARY,
                  create_event_data<MotionInfo>(MotionInfo{m_xCursor, m_yCursor}));
	}
	else {
		// motion on secondary screen.  warp mouse back to
		// center.
		//
		// my lombard (powerbook g3) running linux and
		// using the adbmouse driver has two problems:
		// first, the driver only sends motions of +/-2
		// pixels and, second, it seems to discard some
		// physical input after a warp.  the former isn't a
		// big deal (we're just limited to every other
		// pixel) but the latter is a PITA.  to work around
		// it we only warp when the mouse has moved more
		// than s_size pixels from the center.
		static const std::int32_t s_size = 32;
		if (xmotion.x_root - m_xCenter < -s_size ||
			xmotion.x_root - m_xCenter >  s_size ||
			xmotion.y_root - m_yCenter < -s_size ||
			xmotion.y_root - m_yCenter >  s_size) {
			warpCursorNoFlush(m_xCenter, m_yCenter);
		}

		// send event if mouse moved.  do this after warping
		// back to center in case the motion takes us onto
		// the primary screen.  if we sent the event first
		// in that case then the warp would happen after
		// warping to the primary screen's enter position,
		// effectively overriding it.
		if (x != 0 || y != 0) {
            sendEvent(EventType::PRIMARY_SCREEN_MOTION_ON_SECONDARY,
                      create_event_data<MotionInfo>(MotionInfo{x, y}));
		}
	}
}

int XWindowsScreen::x_accumulateMouseScroll(std::int32_t xDelta) const
{
    m_x_accumulatedScroll += xDelta;
    int numEvents = m_x_accumulatedScroll / m_mouseScrollDelta;
    m_x_accumulatedScroll -= numEvents * m_mouseScrollDelta;
    return numEvents;
}

int XWindowsScreen::y_accumulateMouseScroll(std::int32_t yDelta) const
{
    m_y_accumulatedScroll += yDelta;
    int numEvents = m_y_accumulatedScroll / m_mouseScrollDelta;
    m_y_accumulatedScroll -= numEvents * m_mouseScrollDelta;
    return numEvents;
}

Cursor
XWindowsScreen::createBlankCursor() const
{
	// this seems just a bit more complicated than really necessary

	// get the closet cursor size to 1x1
	unsigned int w = 0, h = 0;
    m_impl->XQueryBestCursor(m_display, m_root, 1, 1, &w, &h);
	w = std::max(1u, w);
	h = std::max(1u, h);

	// make bitmap data for cursor of closet size.  since the cursor
	// is blank we can use the same bitmap for shape and mask:  all
	// zeros.
	const int size = ((w + 7) >> 3) * h;
	char* data = new char[size];
	memset(data, 0, size);

	// make bitmap
    Pixmap bitmap = m_impl->XCreateBitmapFromData(m_display, m_root, data, w, h);

	// need an arbitrary color for the cursor
	XColor color;
	color.pixel = 0;
	color.red   = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;

	// make cursor from bitmap
    Cursor cursor = m_impl->XCreatePixmapCursor(m_display, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	delete[] data;
    m_impl->XFreePixmap(m_display, bitmap);

	return cursor;
}

ClipboardID
XWindowsScreen::getClipboardID(Atom selection) const
{
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        if (m_clipboard[id] != nullptr &&
			m_clipboard[id]->getSelection() == selection) {
			return id;
		}
	}
	return kClipboardEnd;
}

void
XWindowsScreen::processClipboardRequest(Window requestor,
				Time time, Atom property)
{
	// check every clipboard until one returns success
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        if (m_clipboard[id] != nullptr &&
			m_clipboard[id]->processRequest(requestor, time, property)) {
			break;
		}
	}
}

void
XWindowsScreen::destroyClipboardRequest(Window requestor)
{
	// check every clipboard until one returns success
	for (ClipboardID id = 0; id < kClipboardEnd; ++id) {
        if (m_clipboard[id] != nullptr &&
			m_clipboard[id]->destroyRequest(requestor)) {
			break;
		}
	}
}

void
XWindowsScreen::onError()
{
	// prevent further access to the X display
    m_events->adoptBuffer(nullptr);
	m_screensaver->destroy();
    m_screensaver = nullptr;
    m_display = nullptr;

	// notify of failure
    sendEvent(EventType::SCREEN_ERROR, nullptr);

	// FIXME -- should ensure that we ignore operations that involve
	// m_display from now on.  however, Xlib will simply exit the
	// application in response to the X I/O error so there's no
	// point in trying to really handle the error.  if we did want
	// to handle the error, it'd probably be easiest to delegate to
	// one of two objects.  one object would take the implementation
	// from this class.  the other object would be stub methods that
	// don't use X11.  on error, we'd switch to the latter.
}

int
XWindowsScreen::ioErrorHandler(Display*)
{
	// the display has disconnected, probably because X is shutting
	// down.  X forces us to exit at this point which is annoying.
	// we'll pretend as if we won't exit so we try to make sure we
	// don't access the display anymore.
	LOG((CLOG_CRIT "X display has unexpectedly disconnected"));
	s_screen->onError();
	return 0;
}

void
XWindowsScreen::selectEvents(Window w) const
{
	// ignore errors while we adjust event masks.  windows could be
	// destroyed at any time after the XQueryTree() in doSelectEvents()
	// so we must ignore BadWindow errors.
	XWindowsUtil::ErrorLock lock(m_display);

	// adjust event masks
	doSelectEvents(w);
}

void
XWindowsScreen::doSelectEvents(Window w) const
{
	// we want to track the mouse everywhere on the display.  to achieve
	// that we select PointerMotionMask on every window.  we also select
	// SubstructureNotifyMask in order to get CreateNotify events so we
	// select events on new windows too.

	// we don't want to adjust our grab window
	if (w == m_window) {
		return;
	}

       // X11 has a design flaw. If *no* client selected PointerMotionMask for
       // a window, motion events will be delivered to that window's parent.
       // If *any* client, not necessarily the owner, selects PointerMotionMask
       // on such a window, X will stop propagating motion events to its
       // parent. This breaks applications that rely on event propagation
       // behavior.
       //
       // Avoid selecting PointerMotionMask unless some other client selected
       // it already.
       long mask = SubstructureNotifyMask;
       XWindowAttributes attr;
       m_impl->XGetWindowAttributes(m_display, w, &attr);
       if ((attr.all_event_masks & PointerMotionMask) == PointerMotionMask) {
               mask |= PointerMotionMask;
       }

	// select events of interest.  do this before querying the tree so
	// we'll get notifications of children created after the XQueryTree()
	// so we won't miss them.
       m_impl->XSelectInput(m_display, w, mask);

	// recurse on child windows
	Window rw, pw, *cw;
	unsigned int nc;
    if (m_impl->XQueryTree(m_display, w, &rw, &pw, &cw, &nc)) {
		for (unsigned int i = 0; i < nc; ++i) {
			doSelectEvents(cw[i]);
		}
		XFree(cw);
	}
}

KeyID
XWindowsScreen::mapKeyFromX(XKeyEvent* event) const
{
	// convert to a keysym
	KeySym keysym;
    if (event->type == KeyPress && m_ic != nullptr) {
		// do multibyte lookup.  can only call XmbLookupString with a
		// key press event and a valid XIC so we checked those above.
		char scratch[32];
		int n        = sizeof(scratch) / sizeof(scratch[0]);
		char* buffer = scratch;
		int status;
        n = m_impl->XmbLookupString(m_ic, event, buffer, n, &keysym, &status);
		if (status == XBufferOverflow) {
			// not enough space.  grow buffer and try again.
			buffer = new char[n];
            n = m_impl->XmbLookupString(m_ic, event, buffer, n, &keysym, &status);
			delete[] buffer;
		}

		// see what we got.  since we don't care about the string
		// we'll just look for a keysym.
		switch (status) {
		default:
		case XLookupNone:
		case XLookupChars:
			keysym = 0;
			break;

		case XLookupKeySym:
		case XLookupBoth:
			break;
		}
	}
	else {
		// plain old lookup
		char dummy[1];
        m_impl->XLookupString(event, dummy, 0, &keysym, nullptr);
	}

	LOG((CLOG_DEBUG2 "mapped code=%d to keysym=0x%04x", event->keycode, keysym));

	// convert key
	KeyID result = XWindowsUtil::mapKeySymToKeyID(keysym);
	LOG((CLOG_DEBUG2 "mapped keysym=0x%04x to keyID=%d", keysym, result));
	return result;
}

ButtonID
XWindowsScreen::mapButtonFromX(const XButtonEvent* event) const
{
	unsigned int button = event->button;

	// http://xahlee.info/linux/linux_x11_mouse_button_number.html
	// and the program `xev`
	switch (button)
	{
	case 1: case 2: case 3: // kButtonLeft, Middle, Right
		return static_cast<ButtonID>(button);
	case 4: case 5: case 6: case 7: // scroll up, down, left, right -- ignored here
		return kButtonNone;
	case 8: // mouse button 4
		return kButtonExtra0;
	case 9: // mouse button 5
		return kButtonExtra1;
	default: // unknown button
		return kButtonNone;
	}
}

unsigned int
XWindowsScreen::mapButtonToX(ButtonID id) const
{
	switch (id)
	{
	case kButtonLeft: case kButtonMiddle: case kButtonRight:
		return id;
	case kButtonExtra0:
		return 8;
	case kButtonExtra1:
		return 9;
	default:
		return 0;
	}
}

void XWindowsScreen::warpCursorNoFlush(std::int32_t x, std::int32_t y)
{
	assert(m_window != None);

	// send an event that we can recognize before the mouse warp
	XEvent eventBefore;
	eventBefore.type                = MotionNotify;
	eventBefore.xmotion.display     = m_display;
	eventBefore.xmotion.window      = m_window;
	eventBefore.xmotion.root        = m_root;
	eventBefore.xmotion.subwindow   = m_window;
	eventBefore.xmotion.time        = CurrentTime;
	eventBefore.xmotion.x           = x;
	eventBefore.xmotion.y           = y;
	eventBefore.xmotion.x_root      = x;
	eventBefore.xmotion.y_root      = y;
	eventBefore.xmotion.state       = 0;
	eventBefore.xmotion.is_hint     = NotifyNormal;
	eventBefore.xmotion.same_screen = True;
	XEvent eventAfter               = eventBefore;
    m_impl->XSendEvent(m_display, m_window, False, 0, &eventBefore);

	// warp mouse
    m_impl->XWarpPointer(m_display, None, m_root, 0, 0, 0, 0, x, y);

	// send an event that we can recognize after the mouse warp
    m_impl->XSendEvent(m_display, m_window, False, 0, &eventAfter);
    m_impl->XSync(m_display, False);

	LOG((CLOG_DEBUG2 "warped to %d,%d", x, y));
}

void
XWindowsScreen::updateButtons()
{
	// query the button mapping
    std::uint32_t numButtons = m_impl->XGetPointerMapping(m_display, nullptr, 0);
	unsigned char* tmpButtons = new unsigned char[numButtons];
    m_impl->XGetPointerMapping(m_display, tmpButtons, numButtons);

	// find the largest logical button id
	unsigned char maxButton = 0;
    for (std::uint32_t i = 0; i < numButtons; ++i) {
		if (tmpButtons[i] > maxButton) {
			maxButton = tmpButtons[i];
		}
	}

	// allocate button array
	m_buttons.resize(maxButton + 1);

    // fill in button array values.  m_buttons[i] is the physical
    // button number for logical button i.
    std::fill(m_buttons.begin(), m_buttons.end(), 0);

    for (std::uint32_t i = 0; i < numButtons; ++i) {
        m_buttons[tmpButtons[i]] = i;
	}

	// clean up
	delete[] tmpButtons;
}

bool
XWindowsScreen::grabMouseAndKeyboard()
{
	unsigned int event_mask = ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask;

	// grab the mouse and keyboard.  keep trying until we get them.
	// if we can't grab one after grabbing the other then ungrab
	// and wait before retrying.  give up after s_timeout seconds.
	static const double s_timeout = 1.0;
	int result;
	Stopwatch timer;
	do {
		// keyboard first
		do {
            result = m_impl->XGrabKeyboard(m_display, m_window, True,
								GrabModeAsync, GrabModeAsync, CurrentTime);
			assert(result != GrabNotViewable);
			if (result != GrabSuccess) {
				LOG((CLOG_DEBUG2 "waiting to grab keyboard"));
                inputleap::this_thread_sleep(0.05);
				if (timer.getTime() >= s_timeout) {
					LOG((CLOG_DEBUG2 "grab keyboard timed out"));
					return false;
				}
			}
		} while (result != GrabSuccess);
		LOG((CLOG_DEBUG2 "grabbed keyboard"));

		// now the mouse --- use event_mask to get EnterNotify, LeaveNotify events
        result = m_impl->XGrabPointer(m_display, m_window, False, event_mask,
								GrabModeAsync, GrabModeAsync,
								m_window, None, CurrentTime);
		assert(result != GrabNotViewable);
		if (result != GrabSuccess) {
			// back off to avoid grab deadlock
            m_impl->XUngrabKeyboard(m_display, CurrentTime);
			LOG((CLOG_DEBUG2 "ungrabbed keyboard, waiting to grab pointer"));
            inputleap::this_thread_sleep(0.05);
			if (timer.getTime() >= s_timeout) {
				LOG((CLOG_DEBUG2 "grab pointer timed out"));
				return false;
			}
		}
	} while (result != GrabSuccess);

	LOG((CLOG_DEBUG1 "grabbed pointer and keyboard"));
	return true;
}

void
XWindowsScreen::refreshKeyboard(XEvent* event)
{
    if (m_impl->XPending(m_display) > 0) {
		XEvent tmpEvent;
        m_impl->XPeekEvent(m_display, &tmpEvent);
		if (tmpEvent.type == MappingNotify) {
			// discard this event since another follows.
			// we tend to get a bunch of these in a row.
			return;
		}
	}

	// keyboard mapping changed
	if (m_xkb && event->type == m_xkbEventBase) {
        m_impl->XkbRefreshKeyboardMapping(reinterpret_cast<XkbMapNotifyEvent*>(event));
	}
	else
	m_keyState->updateKeyMap();
	m_keyState->updateKeyState();
}


//
// XWindowsScreen::HotKeyItem
//

XWindowsScreen::HotKeyItem::HotKeyItem(int keycode, unsigned int mask) :
	m_keycode(keycode),
	m_mask(mask)
{
	// do nothing
}

bool
XWindowsScreen::HotKeyItem::operator<(const HotKeyItem& x) const
{
	return (m_keycode < x.m_keycode ||
			(m_keycode == x.m_keycode && m_mask < x.m_mask));
}

bool
XWindowsScreen::detectXI2()
{
	int event, error;
    return m_impl->XQueryExtension(m_display,
			"XInputExtension", &xi_opcode, &event, &error);
}

void
XWindowsScreen::selectXIRawMotion()
{
	XIEventMask mask;

	mask.deviceid = XIAllDevices;
	mask.mask_len = XIMaskLen(XI_RawMotion);
    mask.mask = static_cast<unsigned char*>(calloc(mask.mask_len, sizeof(char)));
	mask.deviceid = XIAllMasterDevices;
	memset(mask.mask, 0, 2);
    XISetMask(mask.mask, XI_RawKeyRelease);
	XISetMask(mask.mask, XI_RawMotion);
    m_impl->XISelectEvents(m_display, DefaultRootWindow(m_display), &mask, 1);
	free(mask.mask);
}

} // namespace inputleap
