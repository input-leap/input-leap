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

#ifndef BARRIER_LIB_NET_SECUREUTILS_H
#define BARRIER_LIB_NET_SECUREUTILS_H

#include <string>

namespace barrier {

std::string format_ssl_fingerprint(const std::string& fingerprint,
                                   bool hex = true, bool separator = true);

} // namespace barrier

#endif // BARRIER_LIB_NET_SECUREUTILS_H
