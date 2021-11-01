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

#include "TestUtils.h"
#include <random>

namespace barrier {

std::vector<std::uint8_t> generate_pseudo_random_bytes(std::size_t seed, std::size_t size)
{
    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<int> dist{0, 255};
    std::vector<std::uint8_t> bytes;

    bytes.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        bytes.push_back(dist(engine));
    }

    return bytes;
}

} // namespace barrier
