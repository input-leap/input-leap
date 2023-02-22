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

#include "platform/XWindowsClipboardPNGConverter.h"

namespace inputleap {

//
// XWindowsClipboardPNGConverter
//

XWindowsClipboardPNGConverter::XWindowsClipboardPNGConverter(
                Display* display) :
    m_atom(XInternAtom(display, "image/png", False))
{
    // do nothing
}

XWindowsClipboardPNGConverter::~XWindowsClipboardPNGConverter()
{
    // do nothing
}

IClipboard::EFormat
XWindowsClipboardPNGConverter::getFormat() const
{
    return IClipboard::kPNG;
}

Atom
XWindowsClipboardPNGConverter::getAtom() const
{
    return m_atom;
}

int
XWindowsClipboardPNGConverter::getDataSize() const
{
    return 8;
}

std::string XWindowsClipboardPNGConverter::fromIClipboard(const std::string& pngdata) const
{
    return pngdata;
}

std::string XWindowsClipboardPNGConverter::toIClipboard(const std::string& pngdata) const
{
    if (pngdata.empty()) {
        return {};
    }

    // check PNG file header
    const std::uint8_t* rawPNGHeader = reinterpret_cast<const std::uint8_t*>(pngdata.data());

    if (rawPNGHeader[0] == 0x89 && rawPNGHeader[1] == 0x50 && rawPNGHeader[2] == 0x4e && rawPNGHeader[3] == 0x47) {
        return pngdata;
    }

    return {};
}

} // namespace inputleap
