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

#include "platform/OSXClipboard.h"

//! Convert to/from some text encoding
class OSXClipboardBMPConverter : public IOSXClipboardConverter {
public:
    OSXClipboardBMPConverter();
    virtual ~OSXClipboardBMPConverter();

    // IMSWindowsClipboardConverter overrides
    virtual IClipboard::EFormat
                        getFormat() const;

    virtual CFStringRef
                        getOSXFormat() const;

    // OSXClipboardAnyBMPConverter overrides
    virtual std::string fromIClipboard(const std::string&) const;
    virtual std::string toIClipboard(const std::string&) const;

    // generic encoding converter
    static std::string convertString(const std::string& data, CFStringEncoding fromEncoding,
                                     CFStringEncoding toEncoding);
};
