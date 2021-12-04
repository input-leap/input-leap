/*
    InputLeap -- mouse and keyboard sharing utility
    Copyright (C) InputLeap contributors

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

#ifndef INPUTLEAP_LIB_NET_CONNECTION_SECURITY_LEVEL_H
#define INPUTLEAP_LIB_NET_CONNECTION_SECURITY_LEVEL_H

enum class ConnectionSecurityLevel {
    PLAINTEXT,
    ENCRYPTED,
    ENCRYPTED_AUTHENTICATED
};

#endif // INPUTLEAP_LIB_NET_CONNECTION_SECURITY_LEVEL_H
