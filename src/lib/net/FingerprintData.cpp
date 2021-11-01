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

#include "base/String.h"
#include "FingerprintDatabase.h"
#include "io/filesystem.h"
#include <algorithm>
#include <fstream>

namespace barrier {

bool FingerprintData::operator==(const FingerprintData& other) const
{
    return algorithm == other.algorithm && data == other.data;
}

const char* fingerprint_type_to_string(FingerprintType type)
{
    switch (type) {
        case FingerprintType::INVALID: return "invalid";
        case FingerprintType::SHA1: return "sha1";
        case FingerprintType::SHA256: return "sha256";
    }
    return "invalid";
}

FingerprintType fingerprint_type_from_string(const std::string& type)
{
    if (type == "sha1") {
        return FingerprintType::SHA1;
    }
    if (type == "sha256") {
        return FingerprintType::SHA256;
    }
    return FingerprintType::INVALID;
}

} // namespace barrier
