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

#include "platform/OSXClipboard.h"

#include <gtest/gtest.h>
#include <iostream>

TEST(OSXClipboardTests, empty_openCalled_returnsTrue)
{
    OSXClipboard clipboard;
    clipboard.open(0);

    bool actual = clipboard.empty();

    EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, empty_singleFormat_hasReturnsFalse)
{
    OSXClipboard clipboard;
    clipboard.open(0);
    clipboard.add(OSXClipboard::kText, "barrier rocks!");

    clipboard.empty();

    bool actual = clipboard.has(OSXClipboard::kText);
    EXPECT_EQ(false, actual);
}

TEST(OSXClipboardTests, add_newValue_valueWasStored)
{
    OSXClipboard clipboard;
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "barrier rocks!");

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("barrier rocks!", actual);
}

TEST(OSXClipboardTests, add_replaceValue_valueWasReplaced)
{
    OSXClipboard clipboard;
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "barrier rocks!");
    clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("maxivista sucks", actual);
}

TEST(OSXClipboardTests, open_timeIsZero_returnsTrue)
{
    OSXClipboard clipboard;

    bool actual = clipboard.open(0);

    EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, open_timeIsOne_returnsTrue)
{
    OSXClipboard clipboard;

    bool actual = clipboard.open(1);

    EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, close_isOpen_noErrors)
{
    OSXClipboard clipboard;
    clipboard.open(0);

    clipboard.close();

    // can't assert anything
}

TEST(OSXClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
    OSXClipboard clipboard;
    clipboard.open(1);

    OSXClipboard::Time actual = clipboard.getTime();

    // this behavior is different to that of Clipboard which only
    // returns the value passed into open(t) after empty() is called.
    EXPECT_EQ((std::uint32_t)1, actual);
}

TEST(OSXClipboardTests, getTime_openAndEmpty_returnsOne)
{
    OSXClipboard clipboard;
    clipboard.open(1);
    clipboard.empty();

    OSXClipboard::Time actual = clipboard.getTime();

    EXPECT_EQ((std::uint32_t)1, actual);
}

TEST(OSXClipboardTests, has_withFormatAdded_returnsTrue)
{
    OSXClipboard clipboard;
    clipboard.open(0);
    clipboard.empty();
    clipboard.add(IClipboard::kText, "barrier rocks!");

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_EQ(true, actual);
}

TEST(OSXClipboardTests, has_withNoFormats_returnsFalse)
{
    OSXClipboard clipboard;
    clipboard.open(0);
    clipboard.empty();

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_EQ(false, actual);
}

TEST(OSXClipboardTests, get_withNoFormats_returnsEmpty)
{
    OSXClipboard clipboard;
    clipboard.open(0);
    clipboard.empty();

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("", actual);
}

TEST(OSXClipboardTests, get_withFormatAdded_returnsExpected)
{
    OSXClipboard clipboard;
    clipboard.open(0);
    clipboard.empty();
    clipboard.add(IClipboard::kText, "barrier rocks!");

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("barrier rocks!", actual);
}
