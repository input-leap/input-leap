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

#include "platform/MSWindowsClipboard.h"
#include "platform/IMSWindowsClipboardFacade.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class MSWindowsClipboardTests : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        emptyClipboard();
    }

    virtual void TearDown()
    {
        emptyClipboard();
    }

private:
    void emptyClipboard()
    {
        MSWindowsClipboard clipboard(nullptr);
        clipboard.open(0);
        clipboard.empty();
    }
};

class MockFacade : public IMSWindowsClipboardFacade
{
public:
    MOCK_METHOD2(write, void(HANDLE, UINT));
};

TEST_F(MSWindowsClipboardTests, emptyUnowned_openCalled_returnsTrue)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    bool actual = clipboard.emptyUnowned();

    EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, empty_openCalled_returnsTrue)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    bool actual = clipboard.empty();

    EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, empty_singleFormat_hasReturnsFalse)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);
    clipboard.add(MSWindowsClipboard::kText, "barrier rocks!");

    clipboard.empty();

    bool actual = clipboard.has(MSWindowsClipboard::kText);
    EXPECT_EQ(false, actual);
}

TEST_F(MSWindowsClipboardTests, add_newValue_valueWasStored)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "barrier rocks!");

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("barrier rocks!", actual);
}

TEST_F(MSWindowsClipboardTests, add_newValue_writeWasCalled)
{
    MockFacade facade;
    EXPECT_CALL(facade, write(testing::_, testing::_));

    MSWindowsClipboard clipboard(nullptr);
    clipboard.setFacade(facade);
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "barrier rocks!");
}

TEST_F(MSWindowsClipboardTests, add_replaceValue_valueWasReplaced)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    clipboard.add(IClipboard::kText, "barrier rocks!");
    clipboard.add(IClipboard::kText, "maxivista sucks"); // haha, just kidding.

    std::string actual = clipboard.get(IClipboard::kText);
    EXPECT_EQ("maxivista sucks", actual);
}

TEST_F(MSWindowsClipboardTests, open_timeIsZero_returnsTrue)
{
    MSWindowsClipboard clipboard(nullptr);

    bool actual = clipboard.open(0);

    EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, open_timeIsOne_returnsTrue)
{
    MSWindowsClipboard clipboard(nullptr);

    bool actual = clipboard.open(1);

    EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, close_isOpen_noErrors)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    clipboard.close();

    // can't assert anything
}

// looks like this test may fail intermittently:
// * http://buildbot.symless.com:8000/builders/trunk-win32/builds/246/steps/shell_3/logs/stdio
/*TEST_F(MSWindowsClipboardTests, getTime_openWithNoEmpty_returnsOne)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(1);

    MSWindowsClipboard::Time actual = clipboard.getTime();

    // this behavior is different to that of Clipboard which only
    // returns the value passed into open(t) after empty() is called.
    EXPECT_EQ(1, actual);
}*/

// this also fails intermittently:
// http://buildbot.symless.com:8000/builders/trunk-win32/builds/266/steps/shell_3/logs/stdio
/*TEST_F(MSWindowsClipboardTests, getTime_openAndEmpty_returnsOne)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(1);
    clipboard.empty();

    MSWindowsClipboard::Time actual = clipboard.getTime();

    EXPECT_EQ(1, actual);
}*/

TEST_F(MSWindowsClipboardTests, has_withFormatAdded_returnsTrue)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);
    clipboard.empty();
    clipboard.add(IClipboard::kText, "barrier rocks!");

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_EQ(true, actual);
}

TEST_F(MSWindowsClipboardTests, has_withNoFormats_returnsFalse)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);
    clipboard.empty();

    bool actual = clipboard.has(IClipboard::kText);

    EXPECT_EQ(false, actual);
}

TEST_F(MSWindowsClipboardTests, get_withNoFormats_returnsEmpty)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);
    clipboard.empty();

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("", actual);
}

TEST_F(MSWindowsClipboardTests, get_withFormatAdded_returnsExpected)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);
    clipboard.empty();
    clipboard.add(IClipboard::kText, "barrier rocks!");

    std::string actual = clipboard.get(IClipboard::kText);

    EXPECT_EQ("barrier rocks!", actual);
}

TEST_F(MSWindowsClipboardTests, isOwnedByBarrier_defaultState_noError)
{
    MSWindowsClipboard clipboard(nullptr);
    clipboard.open(0);

    bool actual = clipboard.isOwnedByBarrier();

    EXPECT_EQ(true, actual);
}
