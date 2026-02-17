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
#include <cstdlib>
#include <vector>

namespace inputleap {

// Standalone reimplementation of the isAtOuterEdge logic for testing
// without needing a full Server instance.
static bool isAtOuterEdge(const std::vector<MonitorInfo>& monitors,
                          std::int32_t x, std::int32_t y, EDirection dir)
{
    if (monitors.size() <= 1) {
        return true;
    }

    const MonitorInfo* curMon = nullptr;
    for (const auto& mon : monitors) {
        if (x >= mon.m_x && x < mon.m_x + mon.m_w &&
            y >= mon.m_y && y < mon.m_y + mon.m_h) {
            curMon = &mon;
            break;
        }
    }

    if (curMon == nullptr) {
        return true;
    }

    const std::int32_t kTolerance = 2;

    for (const auto& mon : monitors) {
        if (&mon == curMon) {
            continue;
        }

        switch (dir) {
        case kLeft:
            if (std::abs((mon.m_x + mon.m_w) - curMon->m_x) <= kTolerance &&
                mon.m_y < curMon->m_y + curMon->m_h &&
                mon.m_y + mon.m_h > curMon->m_y) {
                if (y >= mon.m_y && y < mon.m_y + mon.m_h) {
                    return false;
                }
            }
            break;
        case kRight:
            if (std::abs(mon.m_x - (curMon->m_x + curMon->m_w)) <= kTolerance &&
                mon.m_y < curMon->m_y + curMon->m_h &&
                mon.m_y + mon.m_h > curMon->m_y) {
                if (y >= mon.m_y && y < mon.m_y + mon.m_h) {
                    return false;
                }
            }
            break;
        case kTop:
            if (std::abs((mon.m_y + mon.m_h) - curMon->m_y) <= kTolerance &&
                mon.m_x < curMon->m_x + curMon->m_w &&
                mon.m_x + mon.m_w > curMon->m_x) {
                if (x >= mon.m_x && x < mon.m_x + mon.m_w) {
                    return false;
                }
            }
            break;
        case kBottom:
            if (std::abs(mon.m_y - (curMon->m_y + curMon->m_h)) <= kTolerance &&
                mon.m_x < curMon->m_x + curMon->m_w &&
                mon.m_x + mon.m_w > curMon->m_x) {
                if (x >= mon.m_x && x < mon.m_x + mon.m_w) {
                    return false;
                }
            }
            break;
        default:
            break;
        }
    }

    return true;
}

// Helper to create a MonitorInfo
static MonitorInfo makeMon(std::int32_t x, std::int32_t y,
                           std::int32_t w, std::int32_t h,
                           std::uint32_t index)
{
    MonitorInfo info;
    info.m_x = x;
    info.m_y = y;
    info.m_w = w;
    info.m_h = h;
    info.m_index = index;
    return info;
}

// --- Single monitor tests ---

TEST(EdgeDetectionTests, singleMonitor_alwaysOuterEdge)
{
    std::vector<MonitorInfo> monitors = { makeMon(0, 0, 1920, 1080, 0) };

    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 540, kLeft));
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 540, kRight));
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 0, kTop));
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 1079, kBottom));
}

TEST(EdgeDetectionTests, emptyMonitors_alwaysOuterEdge)
{
    std::vector<MonitorInfo> monitors;
    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 0, kLeft));
}

// --- Side-by-side monitors ---

TEST(EdgeDetectionTests, sideBySide_innerSeamSuppressed)
{
    // [Mon0: 0,0 1920x1080] [Mon1: 1920,0 1920x1080]
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(1920, 0, 1920, 1080, 1)
    };

    // Right edge of Mon0 at seam with Mon1 -> inter-monitor seam
    EXPECT_FALSE(isAtOuterEdge(monitors, 1919, 540, kRight));
    // Left edge of Mon1 at seam with Mon0 -> inter-monitor seam
    EXPECT_FALSE(isAtOuterEdge(monitors, 1920, 540, kLeft));
}

TEST(EdgeDetectionTests, sideBySide_outerEdgesAllowed)
{
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(1920, 0, 1920, 1080, 1)
    };

    // Left edge of Mon0 -> true outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 540, kLeft));
    // Right edge of Mon1 -> true outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 3839, 540, kRight));
    // Top/bottom of either -> true outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 0, kTop));
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 1079, kBottom));
    EXPECT_TRUE(isAtOuterEdge(monitors, 2880, 0, kTop));
    EXPECT_TRUE(isAtOuterEdge(monitors, 2880, 1079, kBottom));
}

// --- Stacked monitors ---

TEST(EdgeDetectionTests, stacked_innerSeamSuppressed)
{
    // Mon0 on top, Mon1 below
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(0, 1080, 1920, 1080, 1)
    };

    // Bottom edge of Mon0 at seam with Mon1
    EXPECT_FALSE(isAtOuterEdge(monitors, 960, 1079, kBottom));
    // Top edge of Mon1 at seam with Mon0
    EXPECT_FALSE(isAtOuterEdge(monitors, 960, 1080, kTop));
}

TEST(EdgeDetectionTests, stacked_outerEdgesAllowed)
{
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(0, 1080, 1920, 1080, 1)
    };

    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 0, kTop));
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 2159, kBottom));
    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 540, kLeft));
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 540, kRight));
}

// --- L-shaped layout ---

TEST(EdgeDetectionTests, lShaped_correctEdgeDetection)
{
    // L-shaped: Mon0 top-left, Mon1 top-right, Mon2 bottom-left
    // [Mon0][Mon1]
    // [Mon2]
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(1920, 0, 1920, 1080, 1),
        makeMon(0, 1080, 1920, 1080, 2)
    };

    // Right edge of Mon0 -> seam (Mon1 is there)
    EXPECT_FALSE(isAtOuterEdge(monitors, 1919, 540, kRight));
    // Bottom edge of Mon0 -> seam (Mon2 is there)
    EXPECT_FALSE(isAtOuterEdge(monitors, 960, 1079, kBottom));
    // Right edge of Mon1 -> outer edge (nothing to the right)
    EXPECT_TRUE(isAtOuterEdge(monitors, 3839, 540, kRight));
    // Bottom edge of Mon1 -> outer edge (nothing below Mon1)
    EXPECT_TRUE(isAtOuterEdge(monitors, 2880, 1079, kBottom));
    // Right edge of Mon2 -> outer edge (nothing to the right of Mon2)
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 1620, kRight));
    // Bottom of Mon2 -> outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 960, 2159, kBottom));
}

// --- Monitors with gaps ---

TEST(EdgeDetectionTests, monitorsWithGap_outerEdge)
{
    // Two monitors with a 100px gap between them
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(2020, 0, 1920, 1080, 1)  // 100px gap
    };

    // Right edge of Mon0 -> outer edge (gap is > tolerance)
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 540, kRight));
    // Left edge of Mon1 -> outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 2020, 540, kLeft));
}

TEST(EdgeDetectionTests, monitorsWithSmallGap_seam)
{
    // Two monitors with a 1px gap (within tolerance)
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(1921, 0, 1920, 1080, 1)  // 1px gap
    };

    // Right edge of Mon0 -> seam (within tolerance of 2px)
    EXPECT_FALSE(isAtOuterEdge(monitors, 1919, 540, kRight));
}

// --- Partial vertical overlap ---

TEST(EdgeDetectionTests, partialOverlap_seamOnlyWhereOverlaps)
{
    // Mon0 and Mon1 side by side, but Mon1 is offset vertically
    // Mon0: 0,0 1920x1080
    // Mon1: 1920,500 1920x1080  (overlaps vertically from y=500 to y=1080)
    std::vector<MonitorInfo> monitors = {
        makeMon(0, 0, 1920, 1080, 0),
        makeMon(1920, 500, 1920, 1080, 1)
    };

    // Right edge of Mon0 at y=540 (within overlap) -> seam
    EXPECT_FALSE(isAtOuterEdge(monitors, 1919, 540, kRight));
    // Right edge of Mon0 at y=100 (above Mon1) -> outer edge
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 100, kRight));
}

// --- Backward compatibility ---

TEST(EdgeDetectionTests, singleMonitorFromBoundingRect_matchesLegacy)
{
    // Simulates old behavior: single monitor = bounding rect
    std::vector<MonitorInfo> monitors = { makeMon(-1920, 0, 3840, 1080, 0) };

    EXPECT_TRUE(isAtOuterEdge(monitors, -1920, 540, kLeft));
    EXPECT_TRUE(isAtOuterEdge(monitors, 1919, 540, kRight));
    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 0, kTop));
    EXPECT_TRUE(isAtOuterEdge(monitors, 0, 1079, kBottom));
}

} // namespace inputleap
