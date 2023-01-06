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

#include "platform/XWindowsClipboard.h"

//! Convert to/from UTF-8 encoding
class XWindowsClipboardUTF8Converter : public IXWindowsClipboardConverter {
public:
    /*!
    \c name is converted to an atom and that is reported by getAtom().
    */
    XWindowsClipboardUTF8Converter(Display* display, const char* name);
    ~XWindowsClipboardUTF8Converter() override;

    // IXWindowsClipboardConverter overrides
    IClipboard::EFormat getFormat() const override;
    Atom getAtom() const override;
    int getDataSize() const override;
    std::string fromIClipboard(const std::string&) const override;
    std::string toIClipboard(const std::string&) const override;

private:
    Atom m_atom;
};
