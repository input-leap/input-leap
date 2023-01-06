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

#include "platform/OSXClipboardTextConverter.h"

#include "base/Unicode.h"

//
// OSXClipboardTextConverter
//

OSXClipboardTextConverter::OSXClipboardTextConverter()
{
    // do nothing
}

OSXClipboardTextConverter::~OSXClipboardTextConverter()
{
    // do nothing
}

CFStringRef
OSXClipboardTextConverter::getOSXFormat() const
{
    return CFSTR("public.plain-text");
}

std::string OSXClipboardTextConverter::convertString(const std::string& data,
                                                     CFStringEncoding fromEncoding,
                                                     CFStringEncoding toEncoding)
{
    CFStringRef stringRef =
        CFStringCreateWithCString(kCFAllocatorDefault,
                            data.c_str(), fromEncoding);

    if (stringRef == nullptr) {
        return {};
    }

    CFIndex buffSize;
    CFRange entireString = CFRangeMake(0, CFStringGetLength(stringRef));

    CFStringGetBytes(stringRef, entireString, toEncoding, 0, false, nullptr, 0, &buffSize);

    char* buffer = new char[buffSize];

    if (buffer == nullptr) {
        CFRelease(stringRef);
        return {};
    }

    CFStringGetBytes(stringRef, entireString, toEncoding,
                     0, false, (std::uint8_t*)buffer, buffSize, nullptr);

    std::string result(buffer, buffSize);

    delete[] buffer;
    CFRelease(stringRef);

    return result;
}

std::string OSXClipboardTextConverter::doFromIClipboard(const std::string& data) const
{
    return convertString(data, kCFStringEncodingUTF8,
                            CFStringGetSystemEncoding());
}

std::string OSXClipboardTextConverter::doToIClipboard(const std::string& data) const
{
    return convertString(data, CFStringGetSystemEncoding(),
                            kCFStringEncodingUTF8);
}
