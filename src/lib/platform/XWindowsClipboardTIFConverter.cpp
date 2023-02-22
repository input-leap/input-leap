/*
 * barrier -- mouse and keyboard sharing utility
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

#include "platform/XWindowsClipboardTIFConverter.h"

namespace inputleap {

//
// XWindowsClipboardTIFConverter
//

XWindowsClipboardTIFConverter::XWindowsClipboardTIFConverter(
                Display* display) :
    m_atom(XInternAtom(display, "image/tiff", False))
{
    // do nothing
}

XWindowsClipboardTIFConverter::~XWindowsClipboardTIFConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardTIFConverter::getFormat() const
{
    return IClipboard::kTiff;
}

Atom
XWindowsClipboardTIFConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardTIFConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardTIFConverter::fromIClipboard(const std::string& tiffdata) const
{
    return tiffdata;
}

std::string XWindowsClipboardTIFConverter::toIClipboard(const std::string& tiffdata) const
{
    if (tiffdata.empty()) {
        return {};
    }

    // check TIFF file header, verify if Big or Little Endian
    const std::uint8_t* rawTIFHeader = reinterpret_cast<const std::uint8_t*>(tiffdata.data());

    // Prepare Tiff (BE)
    if (rawTIFHeader[0] == 0x49 && rawTIFHeader[1] == 0x49 && rawTIFHeader[2] == 0x2a && rawTIFHeader[3] == 0x00) {
        return tiffdata;
    }

    // Prepare Tiff (LE)
    if (rawTIFHeader[0] == 0x4D && rawTIFHeader[1] == 0x4D && rawTIFHeader[2] == 0x00 && rawTIFHeader[3] == 0x2a) {
        return tiffdata;
    }

    return {};
}

} // namespace inputleap
