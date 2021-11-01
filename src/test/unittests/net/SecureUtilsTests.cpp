/*
    barrier -- mouse and keyboard sharing utility
    Copyright (C) 2021 Barrier contributors

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

#include "net/SecureUtils.h"

#include "test/global/gtest.h"
#include <random>

namespace {

std::string generate_pseudo_random_bytes(std::size_t seed, std::size_t size)
{
    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<int> dist{0, 255};
    std::vector<char> bytes;

    bytes.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        bytes.push_back(dist(engine));
    }

    return std::string{bytes.data(), bytes.size()};
}

} // namespace

TEST(SecureUtilsTest, FormatSslFingerprintHexWithSeparators)
{
    std::string fingerprint = generate_pseudo_random_bytes(0, 32);
    ASSERT_EQ(format_ssl_fingerprint(fingerprint, true, true),
              "28:FD:0A:98:8A:0E:A1:6C:D7:E8:6C:A7:EE:58:41:71:"
              "CA:B2:8E:49:25:94:90:25:26:05:8D:AF:63:ED:2E:30");
}
