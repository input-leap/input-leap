/*  InputLeap -- mouse and keyboard sharing utility

    InputLeap is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    found in the file LICENSE that should have accompanied this file.

    This package is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Copyright (C) InputLeap developers.
*/

#include "inputleap/DropHelper.h"

#include "base/Log.h"
#include "io/filesystem.h"

#include <fstream>

void
DropHelper::writeToDir(const std::string& destination, DragFileList& fileList, std::string& data)
{
    LOG((CLOG_DEBUG "dropping file, files=%i target=%s", fileList.size(), destination.c_str()));

    if (!destination.empty() && fileList.size() > 0) {
        std::fstream file;
        std::string dropTarget = destination;
#ifdef SYSAPI_WIN32
        dropTarget.append("\\");
#else
        dropTarget.append("/");
#endif
        dropTarget.append(fileList.at(0).getFilename());
        inputleap::open_utf8_path(file, dropTarget, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            LOG((CLOG_ERR "drop file failed: can not open %s", dropTarget.c_str()));
        }

        file.write(data.c_str(), data.size());
        file.close();

        LOG((CLOG_INFO "dropped file \"%s\" in \"%s\"", fileList.at(0).getFilename().c_str(), destination.c_str()));

        fileList.clear();
    }
    else {
        LOG((CLOG_ERR "drop file failed: drop target is empty"));
    }
}
