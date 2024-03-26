/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2014-2016 Symless Ltd.
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

#include "inputleap/DropHelper.h"

#include "base/Log.h"
#include "io/filesystem.h"

#include <fstream>

namespace inputleap {

void
DropHelper::writeToDir(const std::string& destination, DragFileList& fileList, std::string& data)
{
    LOG_DEBUG("dropping file, files=%zi target=%s", fileList.size(), destination.c_str());

    if (!destination.empty() && fileList.size() > 0) {
        std::fstream file;
        std::string dropTarget = destination;
#ifdef SYSAPI_WIN32
        dropTarget.append("\\");
#else
        dropTarget.append("/");
#endif
        dropTarget.append(fileList.at(0).getFilename());
        open_utf8_path(file, dropTarget, std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            LOG_ERR("drop file failed: can not open %s", dropTarget.c_str());
        }

        file.write(data.c_str(), data.size());
        file.close();

        LOG_INFO("dropped file \"%s\" in \"%s\"", fileList.at(0).getFilename().c_str(), destination.c_str());

        fileList.clear();
    }
    else {
        LOG_ERR("drop file failed: drop target is empty");
    }
}

} // namespace inputleap
