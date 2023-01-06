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

#include "base/EventTypes.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#if defined(synwinhk_EXPORTS)
#define CBARRIERHOOK_API __declspec(dllexport)
#else
#define CBARRIERHOOK_API __declspec(dllimport)
#endif

#define INPUTLEAP_MSG_MARK            WM_APP + 0x0011    // mark id; <unused>
#define INPUTLEAP_MSG_KEY             WM_APP + 0x0012    // vk code; key data
#define INPUTLEAP_MSG_MOUSE_BUTTON    WM_APP + 0x0013    // button msg; <unused>
#define INPUTLEAP_MSG_MOUSE_WHEEL     WM_APP + 0x0014    // delta; <unused>
#define INPUTLEAP_MSG_MOUSE_MOVE      WM_APP + 0x0015    // x; y
#define INPUTLEAP_MSG_POST_WARP       WM_APP + 0x0016    // <unused>; <unused>
#define INPUTLEAP_MSG_PRE_WARP        WM_APP + 0x0017    // x; y
#define INPUTLEAP_MSG_SCREEN_SAVER    WM_APP + 0x0018    // activated; <unused>
#define INPUTLEAP_MSG_DEBUG           WM_APP + 0x0019    // data, data
#define INPUTLEAP_MSG_INPUT_FIRST INPUTLEAP_MSG_KEY
#define INPUTLEAP_MSG_INPUT_LAST INPUTLEAP_MSG_PRE_WARP
#define INPUTLEAP_HOOK_LAST_MSG INPUTLEAP_MSG_DEBUG

#define INPUTLEAP_HOOK_FAKE_INPUT_VIRTUAL_KEY    VK_CANCEL
#define INPUTLEAP_HOOK_FAKE_INPUT_SCANCODE    0

extern "C" {

enum EHookMode {
    kHOOK_DISABLE,
    kHOOK_WATCH_JUMP_ZONE,
    kHOOK_RELAY_EVENTS
};

/* REMOVED ImmuneKeys for migration of synwinhk out of DLL

typedef void            (*SetImmuneKeysFunc)(const DWORD*, std::size_t);

// do not call setImmuneKeys() while the hooks are active!
CBARRIERHOOK_API void setImmuneKeys(const DWORD *list, std::size_t size);

*/

}
