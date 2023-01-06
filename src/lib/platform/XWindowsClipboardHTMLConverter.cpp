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

#include "platform/XWindowsClipboardHTMLConverter.h"

#include "base/Unicode.h"

//
// XWindowsClipboardHTMLConverter
//

XWindowsClipboardHTMLConverter::XWindowsClipboardHTMLConverter(
                Display* display, const char* name) :
    m_atom(XInternAtom(display, name, False))
{
    // do nothing
}

XWindowsClipboardHTMLConverter::~XWindowsClipboardHTMLConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardHTMLConverter::getFormat() const
{
    return IClipboard::kHTML;
}

Atom
XWindowsClipboardHTMLConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardHTMLConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardHTMLConverter::fromIClipboard(const std::string& data) const
{
    return data;
}

std::string XWindowsClipboardHTMLConverter::toIClipboard(const std::string& data) const
{
    // Older Firefox [1] and possibly other applications use UTF-16 for text/html - handle both
    // [1] https://bugzilla.mozilla.org/show_bug.cgi?id=1497580
    if (Unicode::isUTF8(data)) {
        return data;
    } else {
        return Unicode::UTF16ToUTF8(data);
    }
    return data;
}
