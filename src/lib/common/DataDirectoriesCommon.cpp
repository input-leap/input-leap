/*  InputLeap -- mouse and keyboard sharing utility
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

#include "DataDirectories.h"

namespace inputleap {

void DataDirectories::maybe_copy_old_profile(const fs::path& old_profile_path,
                                             const fs::path& curr_profile_path)
{
    if (fs::exists(curr_profile_path)) {
        return;
    }
    if (!fs::is_directory(old_profile_path)) {
        return;
    }
    fs::copy(old_profile_path, curr_profile_path, fs::copy_options::recursive);
    auto old_cert_path = curr_profile_path / "SSL" / "Barrier.pem";
    auto new_cert_path = curr_profile_path / "SSL" / "InputLeap.pem";
    if (fs::is_regular_file(old_cert_path) && !fs::exists(new_cert_path)) {
        fs::rename(old_cert_path, new_cert_path);
    }
}

} // namespace inputleap
