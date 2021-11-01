/*
* barrier -- mouse and keyboard sharing utility
* Copyright (C) 2018 Debauchee Open Source Group
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

#ifndef BARRIER_LIB_COMMON_DATA_DIRECTORIES_H
#define BARRIER_LIB_COMMON_DATA_DIRECTORIES_H

#include <string>

namespace barrier {

class DataDirectories
{
public:
    static const std::string& profile();
    static const std::string& profile(const std::string& path);

    static const std::string& global();
    static const std::string& global(const std::string& path);

    static const std::string& systemconfig();
    static const std::string& systemconfig(const std::string& path);

    static std::string ssl_fingerprints_path();
    static std::string local_ssl_fingerprints_path();
    static std::string trusted_servers_ssl_fingerprints_path();
    static std::string trusted_clients_ssl_fingerprints_path();
private:
    static std::string _profile;
    static std::string _global;
    static std::string _systemconfig;
};

} // namespace barrier

#endif
