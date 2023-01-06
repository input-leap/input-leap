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

#include "platform/XWindowsClipboardUTF8Converter.h"

//
// XWindowsClipboardUTF8Converter
//

XWindowsClipboardUTF8Converter::XWindowsClipboardUTF8Converter(
                Display* display, const char* name) :
    m_atom(XInternAtom(display, name, False))
{
    // do nothing
}

XWindowsClipboardUTF8Converter::~XWindowsClipboardUTF8Converter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardUTF8Converter::getFormat() const
{
    return IClipboard::kText;
}

Atom
XWindowsClipboardUTF8Converter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardUTF8Converter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardUTF8Converter::fromIClipboard(const std::string& data) const
{
    return data;
}

std::string XWindowsClipboardUTF8Converter::toIClipboard(const std::string& data) const
{
    return data;
}
