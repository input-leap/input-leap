/*
* InputLeap -- mouse and keyboard sharing utility
* Copyright (C) 2018 The InputLeap Developers
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

#ifndef INPUTLEAP_LIB_COMMON_DATA_DIRECTORIES_H
#define INPUTLEAP_LIB_COMMON_DATA_DIRECTORIES_H

#include "io/filesystem.h"

namespace inputleap {

class DataDirectories
{
public:
    static const fs::path& profile();
    static const fs::path& profile(const fs::path& path);

    static const fs::path& global();
    static const fs::path& global(const fs::path& path);

    static const fs::path& systemconfig();
    static const fs::path& systemconfig(const fs::path& path);

    static fs::path ssl_fingerprints_path();
    static fs::path local_ssl_fingerprints_path();
    static fs::path trusted_servers_ssl_fingerprints_path();
    static fs::path trusted_clients_ssl_fingerprints_path();
    static fs::path ssl_certificate_path();
private:
    static fs::path _profile;
    static fs::path _global;
    static fs::path _systemconfig;
};

} // namespace inputleap

#endif
