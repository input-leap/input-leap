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

#pragma once

#include "inputleap/clipboard_types.h"

#include <string>

class IEventQueue;

class StreamChunker {
public:
    static void sendFile(const char* filename, IEventQueue* events, void* eventTarget);
    static void sendClipboard(std::string& data, std::size_t size, ClipboardID id,
                              std::uint32_t sequence, IEventQueue* events, void* eventTarget);
    static void interruptFile();

private:
    static bool            s_isChunkingFile;
    static bool            s_interruptFile;
};
