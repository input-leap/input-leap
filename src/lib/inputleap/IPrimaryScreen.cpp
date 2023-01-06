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

#include "inputleap/IPrimaryScreen.h"
#include "base/EventQueue.h"

#include <cstdlib>

//
// IPrimaryScreen::ButtonInfo
//

IPrimaryScreen::ButtonInfo*
IPrimaryScreen::ButtonInfo::alloc(ButtonID id, KeyModifierMask mask)
{
    ButtonInfo* info = static_cast<ButtonInfo*>(malloc(sizeof(ButtonInfo)));
    info->m_button = id;
    info->m_mask   = mask;
    return info;
}

IPrimaryScreen::ButtonInfo*
IPrimaryScreen::ButtonInfo::alloc(const ButtonInfo& x)
{
    ButtonInfo* info = static_cast<ButtonInfo*>(malloc(sizeof(ButtonInfo)));
    info->m_button = x.m_button;
    info->m_mask   = x.m_mask;
    return info;
}

bool
IPrimaryScreen::ButtonInfo::equal(const ButtonInfo* a, const ButtonInfo* b)
{
    return (a->m_button == b->m_button && a->m_mask == b->m_mask);
}


//
// IPrimaryScreen::MotionInfo
//

IPrimaryScreen::MotionInfo* IPrimaryScreen::MotionInfo::alloc(std::int32_t x, std::int32_t y)
{
    MotionInfo* info = static_cast<MotionInfo*>(malloc(sizeof(MotionInfo)));
    info->m_x = x;
    info->m_y = y;
    return info;
}


//
// IPrimaryScreen::WheelInfo
//

IPrimaryScreen::WheelInfo* IPrimaryScreen::WheelInfo::alloc(std::int32_t xDelta,
                                                            std::int32_t yDelta)
{
    WheelInfo* info = static_cast<WheelInfo*>(malloc(sizeof(WheelInfo)));
    info->m_xDelta = xDelta;
    info->m_yDelta = yDelta;
    return info;
}


//
// IPrimaryScreen::HotKeyInfo
//

IPrimaryScreen::HotKeyInfo* IPrimaryScreen::HotKeyInfo::alloc(std::uint32_t id)
{
    HotKeyInfo* info = static_cast<HotKeyInfo*>(malloc(sizeof(HotKeyInfo)));
    info->m_id = id;
    return info;
}
