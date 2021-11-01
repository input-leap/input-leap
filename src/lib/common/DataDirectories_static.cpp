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

#include "DataDirectories.h"

namespace barrier {

std::string DataDirectories::_profile;
std::string DataDirectories::_global;
std::string DataDirectories::_systemconfig;

static const char kFingerprintsDirName[] = "SSL/Fingerprints";
static const char kFingerprintsLocalFilename[] = "Local.txt";
static const char kFingerprintsTrustedServersFilename[] = "TrustedServers.txt";
static const char kFingerprintsTrustedClientsFilename[] = "TrustedClients.txt";

std::string DataDirectories::ssl_fingerprints_path()
{
    return profile() + "/" + kFingerprintsDirName;
}

std::string DataDirectories::local_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() + "/" + kFingerprintsLocalFilename;
}

std::string DataDirectories::trusted_servers_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() + "/" + kFingerprintsTrustedServersFilename;
}

std::string DataDirectories::trusted_clients_ssl_fingerprints_path()
{
    return ssl_fingerprints_path() + "/" + kFingerprintsTrustedClientsFilename;
}

} // namespace barrier
