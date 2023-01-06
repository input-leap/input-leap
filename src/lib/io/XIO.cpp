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

#include "io/XIO.h"

//
// XIOClosed
//

std::string XIOClosed::getWhat() const noexcept
{
    return format("XIOClosed", "already closed");
}


//
// XIOEndOfStream
//

std::string XIOEndOfStream::getWhat() const noexcept
{
    return format("XIOEndOfStream", "reached end of stream");
}


//
// XIOWouldBlock
//

std::string XIOWouldBlock::getWhat() const noexcept
{
    return format("XIOWouldBlock", "stream operation would block");
}
