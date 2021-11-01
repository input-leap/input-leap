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

#include "SecureUtils.h"
#include "base/String.h"

namespace barrier {

std::string format_ssl_fingerprint(const std::string& fingerprint, bool hex, bool separator)
{
    std::string result = fingerprint;
    if (hex) {
        // to hexadecimal
        result = barrier::string::to_hex(result, 2);
    }

    // all uppercase
    barrier::string::uppercase(result);

    if (separator) {
        // add colon to separate each 2 characters
        size_t separators = result.size() / 2;
        for (size_t i = 1; i < separators; i++) {
            result.insert(i * 3 - 1, ":");
        }
    }
    return result;
}

} // namespace barrier
