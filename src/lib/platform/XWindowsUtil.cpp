/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2002 Chris Schoeneman
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

#include "platform/XWindowsUtil.h"

#include "mt/Thread.h"
#include "base/Log.h"
#include "base/String.h"

#include <X11/Xatom.h>

namespace inputleap {

//
// XWindowsUtil
//

bool XWindowsUtil::getWindowProperty(Display* display, Window window, Atom property,
                                     std::string* data, Atom* type, std::int32_t* format,
                                     bool deleteProperty)
{
    assert(display != nullptr);

    Atom actualType;
    int actualDatumSize;

    // ignore errors.  XGetWindowProperty() will report failure.
    XWindowsUtil::ErrorLock lock(display);

    // read the property
    bool okay = true;
    const long length = XMaxRequestSize(display);
    long offset = 0;
    unsigned long bytesLeft = 1;
    while (bytesLeft != 0) {
        // get more data
        unsigned long numItems;
        unsigned char* rawData;
        if (XGetWindowProperty(display, window, property,
                                offset, length, False, AnyPropertyType,
                                &actualType, &actualDatumSize,
                                &numItems, &bytesLeft, &rawData) != Success ||
            actualType == None || actualDatumSize == 0) {
            // failed
            okay = false;
            break;
        }

        // compute bytes read and advance offset
        unsigned long numBytes;
        switch (actualDatumSize) {
        case 8:
        default:
            numBytes = numItems;
            offset  += numItems / 4;
            break;

        case 16:
            numBytes = 2 * numItems;
            offset  += numItems / 2;
            break;

        case 32:
            numBytes = 4 * numItems;
            offset  += numItems;
            break;
        }

        // append data
        if (data != nullptr) {
            data->append(reinterpret_cast<char*>(rawData), numBytes);
        }
        else {
            // data is not required so don't try to get any more
            bytesLeft = 0;
        }

        // done with returned data
        XFree(rawData);
    }

    // delete the property if requested
    if (deleteProperty) {
        XDeleteProperty(display, window, property);
    }

    // save property info
    if (type != nullptr) {
        *type = actualType;
    }
    if (format != nullptr) {
        *format = static_cast<std::int32_t>(actualDatumSize);
    }

    if (okay) {
        LOG_DEBUG2("read property %ld on window 0x%08lx: bytes=%zd", property, window, (data == nullptr) ? 0 : data->size());
        return true;
    }
    else {
        LOG_DEBUG2("can't read property %ld on window 0x%08lx", property, window);
        return false;
    }
}

bool XWindowsUtil::setWindowProperty(Display* display, Window window, Atom property,
                                     const void* vdata, std::uint32_t size, Atom type,
                                     std::int32_t format)
{
    const std::uint32_t length = 4 * XMaxRequestSize(display);
    const unsigned char* data = static_cast<const unsigned char*>(vdata);
    std::uint32_t datumSize = static_cast<std::uint32_t>(format / 8);
    // format 32 on 64bit systems is 8 bytes not 4.
    if (format == 32) {
        datumSize = sizeof(Atom);
    }

    // save errors
    bool error = false;
    XWindowsUtil::ErrorLock lock(display, &error);

    // how much data to send in first chunk?
    std::uint32_t chunkSize = size;
    if (chunkSize > length) {
        chunkSize = length;
    }

    // send first chunk
    XChangeProperty(display, window, property,
                                type, format, PropModeReplace,
                                data, chunkSize / datumSize);

    // append remaining chunks
    data += chunkSize;
    size -= chunkSize;
    while (!error && size > 0) {
        chunkSize = size;
        if (chunkSize > length) {
            chunkSize = length;
        }
        XChangeProperty(display, window, property,
                                type, format, PropModeAppend,
                                data, chunkSize / datumSize);
        data += chunkSize;
        size -= chunkSize;
    }

    return !error;
}

Time
XWindowsUtil::getCurrentTime(Display* display, Window window)
{
    XLockDisplay(display);
    // select property events on window
    XWindowAttributes attr;
    XGetWindowAttributes(display, window, &attr);
    XSelectInput(display, window, attr.your_event_mask | PropertyChangeMask);

    // make a property name to receive dummy change
    Atom atom = XInternAtom(display, "TIMESTAMP", False);

    // do a zero-length append to get the current time
    unsigned char dummy;
    XChangeProperty(display, window, atom,
                                XA_INTEGER, 8,
                                PropModeAppend,
                                &dummy, 0);

    // look for property notify events with the following
    PropertyNotifyPredicateInfo filter;
    filter.m_window   = window;
    filter.m_property = atom;

    // wait for reply
    XEvent xevent;
    XIfEvent(display, &xevent, &XWindowsUtil::propertyNotifyPredicate,
                                reinterpret_cast<XPointer>(&filter));
    assert(xevent.type             == PropertyNotify);
    assert(xevent.xproperty.window == window);
    assert(xevent.xproperty.atom   == atom);

    // restore event mask
    XSelectInput(display, window, attr.your_event_mask);
    XUnlockDisplay(display);

    return xevent.xproperty.time;
}

std::string XWindowsUtil::atomToString(Display* display, Atom atom)
{
    if (atom == 0) {
        return "None";
    }

    bool error = false;
    XWindowsUtil::ErrorLock lock(display, &error);
    char* name = XGetAtomName(display, atom);
    if (error) {
        return inputleap::string::sprintf("<UNKNOWN> (%d)", static_cast<int>(atom));
    }
    else {
        std::string msg = inputleap::string::sprintf("%s (%d)", name, static_cast<int>(atom));
        XFree(name);
        return msg;
    }
}

std::string XWindowsUtil::atomsToString(Display* display, const Atom* atom, std::uint32_t num)
{
    char** names = new char*[num];
    bool error = false;
    XWindowsUtil::ErrorLock lock(display, &error);
    XGetAtomNames(display, const_cast<Atom*>(atom), static_cast<int>(num), names);
    std::string msg;
    if (error) {
        for (std::uint32_t i = 0; i < num; ++i) {
            msg += inputleap::string::sprintf("<UNKNOWN> (%d), ", static_cast<int>(atom[i]));
        }
    }
    else {
        for (std::uint32_t i = 0; i < num; ++i) {
            msg += inputleap::string::sprintf("%s (%d), ", names[i], static_cast<int>(atom[i]));
            XFree(names[i]);
        }
    }
    delete[] names;
    if (msg.size() > 2) {
        msg.erase(msg.size() - 2);
    }
    return msg;
}

void XWindowsUtil::convertAtomProperty(std::string& data)
{
    // as best i can tell, 64-bit systems don't pack Atoms into properties
    // as 32-bit numbers but rather as the 64-bit numbers they are.  that
    // seems wrong but we have to cope.  sometimes we'll get a list of
    // atoms that's 8*n+4 bytes long, missing the trailing 4 bytes which
    // should all be 0.  since we're going to reference the Atoms as
    // 64-bit numbers we have to ensure the last number is a full 64 bits.
    if (sizeof(Atom) != 4 && ((data.size() / 4) & 1) != 0) {
        std::uint32_t zero = 0;
        data.append(reinterpret_cast<char*>(&zero), sizeof(zero));
    }
}

void XWindowsUtil::appendAtomData(std::string& data, Atom atom)
{
    data.append(reinterpret_cast<char*>(&atom), sizeof(Atom));
}

void XWindowsUtil::replaceAtomData(std::string& data, std::uint32_t index, Atom atom)
{
    data.replace(index * sizeof(Atom), sizeof(Atom),
                                reinterpret_cast<const char*>(&atom),
                                sizeof(Atom));
}

void XWindowsUtil::appendTimeData(std::string& data, Time time)
{
    data.append(reinterpret_cast<char*>(&time), sizeof(Time));
}

Bool
XWindowsUtil::propertyNotifyPredicate(Display*, XEvent* xevent, XPointer arg)
{
    PropertyNotifyPredicateInfo* filter =
                        reinterpret_cast<PropertyNotifyPredicateInfo*>(arg);
    return (xevent->type             == PropertyNotify &&
            xevent->xproperty.window == filter->m_window &&
            xevent->xproperty.atom   == filter->m_property &&
            xevent->xproperty.state  == PropertyNewValue) ? True : False;
}


//
// XWindowsUtil::ErrorLock
//

XWindowsUtil::ErrorLock* XWindowsUtil::ErrorLock::s_top = nullptr;

XWindowsUtil::ErrorLock::ErrorLock(Display* display) :
    m_display(display)
{
    install(&XWindowsUtil::ErrorLock::ignoreHandler, nullptr);
}

XWindowsUtil::ErrorLock::ErrorLock(Display* display, bool* flag) :
    m_display(display)
{
    install(&XWindowsUtil::ErrorLock::saveHandler, flag);
}

XWindowsUtil::ErrorLock::ErrorLock(Display* display,
                ErrorHandler handler, void* data) :
    m_display(display)
{
    install(handler, data);
}

XWindowsUtil::ErrorLock::~ErrorLock()
{
    // make sure everything finishes before uninstalling handler
    if (m_display != nullptr) {
        XSync(m_display, False);
    }

    // restore old handler
    XSetErrorHandler(m_oldXHandler);
    s_top = m_next;
}

void
XWindowsUtil::ErrorLock::install(ErrorHandler handler, void* data)
{
    // make sure everything finishes before installing handler
    if (m_display != nullptr) {
        XSync(m_display, False);
    }

    // install handler
    m_handler     = handler;
    m_userData    = data;
    m_oldXHandler = XSetErrorHandler(
                                &XWindowsUtil::ErrorLock::internalHandler);
    m_next        = s_top;
    s_top         = this;
}

int
XWindowsUtil::ErrorLock::internalHandler(Display* display, XErrorEvent* event)
{
    if (s_top != nullptr && s_top->m_handler != nullptr) {
        s_top->m_handler(display, event, s_top->m_userData);
    }
    return 0;
}

void
XWindowsUtil::ErrorLock::ignoreHandler(Display* display, XErrorEvent* e, void*)
{
    char errtxt[1024];
    XGetErrorText(display, e->error_code, errtxt, 1023);
    LOG_DEBUG1("ignoring X error: %d - %.1023s", e->error_code, errtxt);
}

void
XWindowsUtil::ErrorLock::saveHandler(Display* display, XErrorEvent* e, void* flag)
{
    char errtxt[1024];
    XGetErrorText(display, e->error_code, errtxt, 1023);
    LOG_DEBUG1("flagging X error: %d - %.1023s", e->error_code, errtxt);
    *static_cast<bool*>(flag) = true;
}

} // namespace inputleap
