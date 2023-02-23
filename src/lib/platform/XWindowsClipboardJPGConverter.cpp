/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2023 Draekko
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2004 Chris Schoeneman
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

#include "platform/XWindowsClipboardJPGConverter.h"

namespace inputleap {

//
// XWindowsClipboardJPGConverter
//

XWindowsClipboardJPGConverter::XWindowsClipboardJPGConverter(
                Display* display) :
    m_atom(XInternAtom(display, "image/jpeg", False))
{
    // do nothing
}

XWindowsClipboardJPGConverter::~XWindowsClipboardJPGConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardJPGConverter::getFormat() const
{
    return IClipboard::kJpeg;
}

Atom
XWindowsClipboardJPGConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardJPGConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardJPGConverter::fromIClipboard(const std::string& jpegdata) const
{
    return jpegdata;
}

std::string XWindowsClipboardJPGConverter::toIClipboard(const std::string& jpegdata) const
{
    if (jpegdata.empty()) {
        return {};
    }
    
    // check JPG file header
    const std::uint8_t* rawJPGHeader = reinterpret_cast<const std::uint8_t*>(jpegdata.data());
    
    if (rawJPGHeader[0] == 0xFF && rawJPGHeader[1] == 0xD8 && rawJPGHeader[2] == 0xFF) {
        return jpegdata;
    }

    return {};
}

} // namespace inputleap
