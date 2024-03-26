/*
 * InputLeap -- mouse and keyboard sharing utility
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

#include "platform/OSXClipboard.h"

#include "inputleap/Clipboard.h"
#include "platform/OSXClipboardUTF16Converter.h"
#include "platform/OSXClipboardTextConverter.h"
#include "platform/OSXClipboardBMPConverter.h"
#include "platform/OSXClipboardHTMLConverter.h"
#include "base/Log.h"
#include "arch/XArch.h"

namespace inputleap {

OSXClipboard::OSXClipboard() :
    m_time(0),
    m_pboard(nullptr)
{
    m_converters.push_back(new OSXClipboardHTMLConverter);
    m_converters.push_back(new OSXClipboardBMPConverter);
    m_converters.push_back(new OSXClipboardUTF16Converter);
    m_converters.push_back(new OSXClipboardTextConverter);



    OSStatus createErr = PasteboardCreate(kPasteboardClipboard, &m_pboard);
    if (createErr != noErr) {
        LOG_DEBUG("failed to create clipboard reference: error %i", createErr);
        LOG_ERR("unable to connect to pasteboard, clipboard sharing disabled");
        m_pboard = nullptr;
        return;

    }

    OSStatus syncErr = PasteboardSynchronize(m_pboard);
    if (syncErr != noErr) {
        LOG_DEBUG("failed to synchronize clipboard: error %i", syncErr);
    }
}

OSXClipboard::~OSXClipboard()
{
    clearConverters();
}

    bool
OSXClipboard::empty()
{
    LOG_DEBUG("emptying clipboard");
    if (m_pboard == nullptr)
        return false;

    OSStatus err = PasteboardClear(m_pboard);
    if (err != noErr) {
        LOG_DEBUG("failed to clear clipboard: error %i", err);
        return false;
    }

    return true;
}

    bool
OSXClipboard::synchronize()
{
    if (m_pboard == nullptr)
        return false;

    PasteboardSyncFlags flags = PasteboardSynchronize(m_pboard);
    LOG_DEBUG2("flags: %x", flags);

    if (flags & kPasteboardModified) {
        return true;
    }
    return false;
}

void OSXClipboard::add(EFormat format, const std::string& data)
{
    if (m_pboard == nullptr)
        return;

    LOG_DEBUG("add %zd bytes to clipboard format: %d", data.size(), format);
    if (format == IClipboard::kText) {
        LOG_DEBUG(" format of data to be added to clipboard was kText");
    }
    else if (format == IClipboard::kBitmap) {
        LOG_DEBUG(" format of data to be added to clipboard was kBitmap");
    }
    else if (format == IClipboard::kHTML) {
        LOG_DEBUG(" format of data to be added to clipboard was kHTML");
    }

    for (ConverterList::const_iterator index = m_converters.begin();
            index != m_converters.end(); ++index) {

        IOSXClipboardConverter* converter = *index;

        // skip converters for other formats
        if (converter->getFormat() == format) {
            std::string osXData = converter->fromIClipboard(data);
            CFStringRef flavorType = converter->getOSXFormat();
            CFDataRef dataRef = CFDataCreate(kCFAllocatorDefault, (std::uint8_t *)osXData.data(),
                                             osXData.size());
            PasteboardItemID itemID = 0;

            PasteboardPutItemFlavor(
                m_pboard,
                itemID,
                flavorType,
                dataRef,
                kPasteboardFlavorNoFlags);

            LOG_DEBUG("added %zd bytes to clipboard format: %d", data.size(), format);
        }

    }
}

bool
OSXClipboard::open(Time time) const
{
    if (m_pboard == nullptr)
        return false;

    LOG_DEBUG("opening clipboard");
    m_time = time;
    return true;
}

void
OSXClipboard::close() const
{
    LOG_DEBUG("closing clipboard");
    /* not needed */
}

IClipboard::Time
OSXClipboard::getTime() const
{
    return m_time;
}

bool
OSXClipboard::has(EFormat format) const
{
    if (m_pboard == nullptr)
        return false;

    PasteboardItemID item;
    PasteboardGetItemIdentifier(m_pboard, (CFIndex) 1, &item);

    for (ConverterList::const_iterator index = m_converters.begin();
            index != m_converters.end(); ++index) {
        IOSXClipboardConverter* converter = *index;
        if (converter->getFormat() == format) {
            PasteboardFlavorFlags flags;
            CFStringRef type = converter->getOSXFormat();

            OSStatus res;

            if ((res = PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags)) == noErr) {
                return true;
            }
        }
    }

    return false;
}

std::string OSXClipboard::get(EFormat format) const
{
    CFStringRef type;
    PasteboardItemID item;
    std::string result;

    if (m_pboard == nullptr)
        return result;

    PasteboardGetItemIdentifier(m_pboard, (CFIndex) 1, &item);


    // find the converter for the first clipboard format we can handle
    IOSXClipboardConverter* converter = nullptr;
    for (ConverterList::const_iterator index = m_converters.begin();
            index != m_converters.end(); ++index) {
        converter = *index;

        PasteboardFlavorFlags flags;
        type = converter->getOSXFormat();

        if (converter->getFormat() == format &&
                PasteboardGetItemFlavorFlags(m_pboard, item, type, &flags) == noErr) {
            break;
        }
        converter = nullptr;
    }

    // if no converter then we don't recognize any formats
    if (converter == nullptr) {
        LOG_DEBUG("Unable to find converter for data");
        return result;
    }

    // get the clipboard data.
    CFDataRef buffer = nullptr;
    try {
        OSStatus err = PasteboardCopyItemFlavorData(m_pboard, item, type, &buffer);

        if (err != noErr) {
            throw err;
        }

        result = std::string((char *) CFDataGetBytePtr(buffer), CFDataGetLength(buffer));
    }
    catch (OSStatus err) {
        LOG_DEBUG("exception thrown in OSXClipboard::get MacError (%d)", err);
    }
    catch (...) {
        LOG_DEBUG("unknown exception in OSXClipboard::get");
        RETHROW_XTHREAD
    }

    if (buffer != nullptr)
        CFRelease(buffer);

    return converter->toIClipboard(result);
}

    void
OSXClipboard::clearConverters()
{
    if (m_pboard == nullptr)
        return;

    for (ConverterList::iterator index = m_converters.begin();
            index != m_converters.end(); ++index) {
        delete *index;
    }
    m_converters.clear();
}

} // namespace inputleap
