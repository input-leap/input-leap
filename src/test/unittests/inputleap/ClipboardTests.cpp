/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Nick Bolton
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

#include "inputleap/Clipboard.h"

#include <gtest/gtest.h>

namespace inputleap {

TEST(ClipboardTests, empty_openCalled_returnsTrue)
{
    Clipboard clipboard;
    clipboard.open(0);

    bool actual = clipboard.empty();

    EXPECT_EQ(true, actual);
}

TEST(ClipboardTests, empty_singleFormat_hasReturnsFalse)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(Clipboard::kText, "test string!");

    clipboard.empty();

    bool actual = clipboard.has(Clipboard::kText);
    EXPECT_FALSE(actual);
}

TEST(ClipboardTests, add_newValue_valueWasStored)
{
    Clipboard clipboard;
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "test string!");

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("test string!", actual);
}

TEST(ClipboardTests, add_replaceValue_valueWasReplaced)
{
    Clipboard clipboard;
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "test string!");
    clipboard.add(IClipboard::kText, "other string");

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("other string", actual);
}

TEST(ClipboardTests, open_timeIsZero_returnsTrue)
{
    Clipboard clipboard;

    bool actual = clipboard.open(0);

    EXPECT_EQ(true, actual);
}

TEST(ClipboardTests, open_timeIsOne_returnsTrue)
{
    Clipboard clipboard;

    bool actual = clipboard.open(1);

    EXPECT_EQ(true, actual);
}

TEST(ClipboardTests, close_isOpen_noErrors)
{
    Clipboard clipboard;
    clipboard.open(0);

    clipboard.close();

    // can't assert anything
}

TEST(ClipboardTests, getTime_openWithNoEmpty_returnsZero)
{
    Clipboard clipboard;
    clipboard.open(1);

    Clipboard::Time actual = clipboard.getTime();

    EXPECT_EQ(0u, actual);
}

TEST(ClipboardTests, getTime_openAndEmpty_returnsOne)
{
    Clipboard clipboard;
    clipboard.open(1);
    clipboard.empty();

    Clipboard::Time actual = clipboard.getTime();

    EXPECT_EQ(1u, actual);
}

TEST(ClipboardTests, has_withFormatAdded_returnsTrue)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string!");

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_EQ(true, actual);
}

TEST(ClipboardTests, has_withNoFormats_returnsFalse)
{
    Clipboard clipboard;
    clipboard.open(0);

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_FALSE(actual);
}

TEST(ClipboardTests, get_withNoFormats_returnsEmpty)
{
    Clipboard clipboard;
    clipboard.open(0);

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("", actual);
}

TEST(ClipboardTests, get_withFormatAdded_returnsExpected)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string!");

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("test string!", actual);
}

TEST(ClipboardTests, marshall_addNotCalled_firstCharIsZero)
{
    Clipboard clipboard;

    std::string actual = clipboard.marshall();

    // seems to return "\0\0\0\0" but EXPECT_EQ can't assert this,
    // so instead, just assert that first char is '\0'.
    EXPECT_EQ(0, static_cast<int>(actual[0]));
}

TEST(ClipboardTests, marshall_withTextAdded_typeCharIsText)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string!");
    clipboard.close();

    std::string actual = clipboard.marshall();

    // string contains other data, but 8th char should be kText.
    EXPECT_EQ(IClipboard::kText, static_cast<int>(actual[7]));
}

TEST(ClipboardTests, marshall_withTextAdded_lastSizeCharIs14)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string!"); // 12 chars
    clipboard.close();

    std::string actual = clipboard.marshall();

    EXPECT_EQ(12, static_cast<int>(actual[11]));
}

// TODO: there's some integer -> char encoding going on here. i find it
// hard to believe that the clipboard is the only thing doing this. maybe
// we should refactor this stuff out of the clipboard.
TEST(ClipboardTests, marshall_withTextSize289_sizeCharsValid)
{
    // 289 chars
    std::string data;
    data.append("InputLeap is Free and Open Source Software that lets you ");
    data.append("easily share your mouse and keyboard between multiple ");
    data.append("computers, where each computer has it's own display. No ");
    data.append("special hardware is required, all you need is a local area ");
    data.append("network. InputLeap is supported on Windows, Mac OS X and Linux.");

    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, data);
    clipboard.close();

    std::string actual = clipboard.marshall();

    EXPECT_EQ(0, actual[8]); // 289 >> 24
    EXPECT_EQ(0, actual[9]); // 289 >> 16
    EXPECT_EQ(1, actual[10]); // 289 >> 8
    EXPECT_EQ(33, actual[11]); // 289 & 0xff
}

TEST(ClipboardTests, marshall_withHtmlAdded_typeCharIsHtml)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kHTML, "other test string!");
    clipboard.close();

    std::string actual = clipboard.marshall();

    // string contains other data, but 8th char should be kHTML.
    EXPECT_EQ(IClipboard::kHTML, static_cast<int>(actual[7]));
}

TEST(ClipboardTests, marshall_withHtmlAndText_has2Formats)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string");
    clipboard.add(IClipboard::kHTML, "other test string!");
    clipboard.close();

    std::string actual = clipboard.marshall();

    // the number of formats is stored inside the first 4 chars.
    // the writeUInt32 function right-aligns numbers in 4 chars,
    // so if you right align 2, it will be "\0\0\0\2" in a string.
    // we assert that the char at the 4th index is 2 (the number of
    // formats that we've added).
    EXPECT_EQ(2, static_cast<int>(actual[3]));
}

TEST(ClipboardTests, marshall_withTextAdded_endsWithAdded)
{
    Clipboard clipboard;
    clipboard.open(0);
    clipboard.add(IClipboard::kText, "test string!");
    clipboard.close();

    std::string actual = clipboard.marshall();

    // string contains other data, but should end in the string we added.
    EXPECT_EQ("test string!", actual.substr(12));
}

TEST(ClipboardTests, unmarshall_emptyData_hasTextIsFalse)
{
    Clipboard clipboard;

    std::string data;
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0); // 0 formats added

    clipboard.unmarshall(data, 0);

    clipboard.open(0);
    bool actual = clipboard.has(IClipboard::kText);
    EXPECT_FALSE(actual);
}

TEST(ClipboardTests, unmarshall_withTextSize289_getTextIsValid)
{
    Clipboard clipboard;

    // 289 chars
    std::string text;
    text.append("InputLeap is Free and Open Source Software that lets you ");
    text.append("easily share your mouse and keyboard between multiple ");
    text.append("computers, where each computer has it's own display. No ");
    text.append("special hardware is required, all you need is a local area ");
    text.append("network. InputLeap is supported on Windows, Mac OS X and Linux.");

    std::string data;
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(1); // 1 format added
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(IClipboard::kText);
    data += static_cast<char>(0); // 289 >> 24
    data += static_cast<char>(0); // 289 >> 16
    data += static_cast<char>(1); // 289 >> 8
    data += static_cast<char>(33); // 289 & 0xff = 33
    data += text;

    clipboard.unmarshall(data, 0);

    clipboard.open(0);
    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ(text, actual);
}

TEST(ClipboardTests, unmarshall_withTextAndHtml_getTextIsValid)
{
    Clipboard clipboard;
    std::string data;
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(2); // 2 formats added
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(IClipboard::kText);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(12);
    data += "test string!";
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(IClipboard::kHTML);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(18);
    data += "other test string!";

    clipboard.unmarshall(data, 0);

    clipboard.open(0);
    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("test string!", actual);
}

TEST(ClipboardTests, unmarshall_withTextAndHtml_getHtmlIsValid)
{
    Clipboard clipboard;
    std::string data;
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(2); // 2 formats added
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(IClipboard::kText);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(12);
    data += "test string!";
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(IClipboard::kHTML);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(0);
    data += static_cast<char>(18);
    data += "other test string!";

    clipboard.unmarshall(data, 0);

    clipboard.open(0);
    std::string actual = clipboard.get(IClipboard::kHTML);
    EXPECT_EQ("other test string!", actual);
}

TEST(ClipboardTests, copy_withSingleText_clipboardsAreEqual)
{
    Clipboard clipboard1;
    clipboard1.open(0);
    clipboard1.add(Clipboard::kText, "test string!");
    clipboard1.close();

    Clipboard clipboard2;
    Clipboard::copy(&clipboard2, &clipboard1);

    clipboard2.open(0);
    std::string actual = clipboard2.get(Clipboard::kText);
    EXPECT_EQ("test string!", actual);
}

} // namespace inputleap
