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

#include "../DataDirectories.h"
#include "encoding_utilities.h"

#include <Shlobj.h>

namespace barrier {

fs::path known_folder_path(const KNOWNFOLDERID& id)
{
    fs::path path;
    WCHAR* buffer;
    HRESULT result = SHGetKnownFolderPath(id, 0, NULL, &buffer);
    if (result == S_OK) {
        path = fs::path(std::wstring(buffer));
        CoTaskMemFree(buffer);
    }
    return path;
}

const fs::path& DataDirectories::profile()
{
    if (_profile.empty())
        _profile = known_folder_path(FOLDERID_LocalAppData) / "Barrier";
    return _profile;
}
const fs::path& DataDirectories::profile(const fs::path& path)
{
    _profile = path;
    return _profile;
}

const fs::path& DataDirectories::global()
{
    if (_global.empty())
        _global = known_folder_path(FOLDERID_ProgramData) / "Barrier";
    return _global;
}
const fs::path& DataDirectories::global(const fs::path& path)
{
    _global = path;
    return _global;
}

const fs::path& DataDirectories::systemconfig()
{
    // systemconfig() is a special case in that it will track the current value
    // of global() unless and until it is explicitly set otherwise
    // previously it would default to the windows folder which was horrible!
    if (_systemconfig.empty())
        return global();
    return _systemconfig;
}

const fs::path& DataDirectories::systemconfig(const fs::path& path)
{
    _systemconfig = path;
    return _systemconfig;
}

} // namespace barrier
