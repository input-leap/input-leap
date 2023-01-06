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

#include "OSXClipboardAnyTextConverter.h"

//! Convert to/from HTML encoding
class OSXClipboardHTMLConverter : public OSXClipboardAnyTextConverter {
public:
    OSXClipboardHTMLConverter();
    virtual ~OSXClipboardHTMLConverter();

    // IMSWindowsClipboardConverter overrides
    virtual IClipboard::EFormat
                        getFormat() const;

    virtual CFStringRef getOSXFormat() const;

protected:
    // OSXClipboardAnyTextConverter overrides
    virtual std::string doFromIClipboard(const std::string&) const;
    virtual std::string doToIClipboard(const std::string&) const;

    // generic encoding converter
    static std::string convertString(const std::string& data, CFStringEncoding fromEncoding,
                                     CFStringEncoding toEncoding);
};
