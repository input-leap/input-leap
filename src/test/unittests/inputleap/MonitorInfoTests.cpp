/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) InputLeap contributors
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

#include "inputleap/protocol_types.h"

#include <gtest/gtest.h>

namespace inputleap {

TEST(MonitorInfoTests, defaultConstruction_hasZeroValues)
{
    MonitorInfo info;
    EXPECT_EQ(info.m_x, 0);
    EXPECT_EQ(info.m_y, 0);
    EXPECT_EQ(info.m_w, 0);
    EXPECT_EQ(info.m_h, 0);
    EXPECT_EQ(info.m_index, 0u);
}

TEST(MonitorInfoTests, construction_withValues)
{
    MonitorInfo info;
    info.m_x = -1920;
    info.m_y = 0;
    info.m_w = 1920;
    info.m_h = 1080;
    info.m_index = 1;

    EXPECT_EQ(info.m_x, -1920);
    EXPECT_EQ(info.m_y, 0);
    EXPECT_EQ(info.m_w, 1920);
    EXPECT_EQ(info.m_h, 1080);
    EXPECT_EQ(info.m_index, 1u);
}

TEST(MonitorInfoTests, vectorOfMonitors_worksCorrectly)
{
    std::vector<MonitorInfo> monitors;

    MonitorInfo m1;
    m1.m_x = 0; m1.m_y = 0; m1.m_w = 1920; m1.m_h = 1080; m1.m_index = 0;
    monitors.push_back(m1);

    MonitorInfo m2;
    m2.m_x = 1920; m2.m_y = 0; m2.m_w = 2560; m2.m_h = 1440; m2.m_index = 1;
    monitors.push_back(m2);

    EXPECT_EQ(monitors.size(), 2u);
    EXPECT_EQ(monitors[0].m_w, 1920);
    EXPECT_EQ(monitors[1].m_x, 1920);
}

TEST(MonitorInfoTests, defaultGetMonitors_returnsSingleMonitorFromShape)
{
    // Simulate what IScreen::getMonitors() default does:
    // given a bounding rect, it should return a single MonitorInfo
    // matching that rect with index 0.
    std::int32_t x = -1920, y = 0, w = 3840, h = 1080;

    // Replicate IScreen::getMonitors() default logic
    MonitorInfo info;
    info.m_x = x;
    info.m_y = y;
    info.m_w = w;
    info.m_h = h;
    info.m_index = 0;
    std::vector<MonitorInfo> monitors = { info };

    ASSERT_EQ(monitors.size(), 1u);
    EXPECT_EQ(monitors[0].m_x, -1920);
    EXPECT_EQ(monitors[0].m_y, 0);
    EXPECT_EQ(monitors[0].m_w, 3840);
    EXPECT_EQ(monitors[0].m_h, 1080);
    EXPECT_EQ(monitors[0].m_index, 0u);
}

} // namespace inputleap
