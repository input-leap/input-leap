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

#include <unistd.h>    // sysconf
#include <cstdlib>     // getenv
#include <sys/types.h> // getpwuid(_r)
#include <pwd.h>       // getpwuid(_r)

namespace barrier {

static std::string pw_dir(struct passwd* pwentp)
{
    if (pwentp != NULL && pwentp->pw_dir != NULL)
        return pwentp->pw_dir;
    return "";
}

#ifdef HAVE_GETPWUID_R

static fs::path unix_home()
{
    long size = -1;
#if defined(_SC_GETPW_R_SIZE_MAX)
    size = sysconf(_SC_GETPW_R_SIZE_MAX);
#endif
    if (size == -1)
        size = BUFSIZ;

    struct passwd pwent;
    struct passwd* pwentp;
    std::string buffer(size, 0);
    getpwuid_r(getuid(), &pwent, &buffer[0], size, &pwentp);
    return fs::u8path(pw_dir(pwentp));
}

#else // not HAVE_GETPWUID_R

static fs::path unix_home()
{
    return fs::u8path(pw_dir(getpwuid(getuid())));
}

#endif // HAVE_GETPWUID_R

static fs::path profile_basedir()
{
#ifdef WINAPI_XWINDOWS
    // linux/bsd adheres to freedesktop standards
    // https://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
    const char* dir = std::getenv("XDG_DATA_HOME");
    if (dir != NULL)
        return fs::u8path(dir);
    return unix_home() / ".local/share";
#else
    // macos has its own standards
    // https://developer.apple.com/library/content/documentation/General/Conceptual/MOSXAppProgrammingGuide/AppRuntime/AppRuntime.html
    return unix_home() / "Library/Application Support";
#endif
}

const fs::path& DataDirectories::profile()
{
    if (_profile.empty())
        _profile = profile_basedir() / "barrier";
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
        // TODO: where on a unix system should public/global shared data go?
        // as of march 2018 global() is not used for unix
        _global = "/tmp";
    return _global;
}
const fs::path& DataDirectories::global(const fs::path& path)
{
    _global = path;
    return _global;
}

const fs::path& DataDirectories::systemconfig()
{
    if (_systemconfig.empty())
        _systemconfig = "/etc";
    return _systemconfig;
}

const fs::path& DataDirectories::systemconfig(const fs::path& path)
{
    _systemconfig = path;
    return _systemconfig;
}

} // namespace barrier
