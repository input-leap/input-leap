/*
    barrier -- mouse and keyboard sharing utility
    Copyright (C) Barrier contributors

    This package is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "net/FingerprintDatabase.h"
#include "test/global/gtest.h"

namespace barrier {

TEST(FingerprintDatabase, parse_db_line)
{
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("").valid());
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("abcd").valid());
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("v1:algo:something").valid());
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("v2:algo:something").valid());
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("v2:algo:01020304abc").valid());
    ASSERT_FALSE(FingerprintDatabase::parse_db_line("v2:algo:01020304ZZ").valid());
    ASSERT_EQ(FingerprintDatabase::parse_db_line("v2:algo:01020304ab"),
              (FingerprintData{"algo", {1, 2, 3, 4, 0xab}}));
}

TEST(FingerprintDatabase, read)
{
    std::istringstream stream;
    stream.str(R"(
v2:algo1:01020304ab
v2:algo2:03040506ab
AB:CD:EF:00:01:02:03:04:05:06:07:08:09:10:11:12:13:14:15:16
)");
    FingerprintDatabase db;
    db.read_stream(stream);

    std::vector<FingerprintData> expected = {
        { "algo1", { 1, 2, 3, 4, 0xab } },
        { "algo2", { 3, 4, 5, 6, 0xab } },
        { "sha1", { 0xab, 0xcd, 0xef, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 } },
    };
    ASSERT_EQ(db.fingerprints(), expected);
}

TEST(FingerprintDatabase, write)
{
    std::ostringstream stream;

    FingerprintDatabase db;
    db.add_trusted({ "algo1", { 1, 2, 3, 4, 0xab } });
    db.add_trusted({ "algo2", { 3, 4, 5, 6, 0xab } });
    db.write_stream(stream);

    ASSERT_EQ(stream.str(), R"(v2:algo1:01020304ab
v2:algo2:03040506ab
)");
}

TEST(FingerprintDatabase, clear)
{
    FingerprintDatabase db;
    db.add_trusted({ "algo1", { 1, 2, 3, 4, 0xab } });
    db.clear();
    ASSERT_TRUE(db.fingerprints().empty());
}

TEST(FingerprintDatabase, add_trusted_no_duplicates)
{
    FingerprintDatabase db;
    db.add_trusted({ "algo1", { 1, 2, 3, 4, 0xab } });
    db.add_trusted({ "algo2", { 3, 4, 5, 6, 0xab } });
    db.add_trusted({ "algo1", { 1, 2, 3, 4, 0xab } });
    ASSERT_EQ(db.fingerprints().size(), 2);
}

TEST(FingerprintDatabase, is_trusted)
{
    FingerprintDatabase db;
    db.add_trusted({ "algo1", { 1, 2, 3, 4, 0xab } });
    ASSERT_TRUE(db.is_trusted({ "algo1", { 1, 2, 3, 4, 0xab } }));
    ASSERT_FALSE(db.is_trusted({ "algo2", { 1, 2, 3, 4, 0xab } }));
    ASSERT_FALSE(db.is_trusted({ "algo1", { 1, 2, 3, 4, 0xac } }));
}

} // namespace barrier
