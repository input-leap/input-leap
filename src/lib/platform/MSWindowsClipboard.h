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

#include "platform/MSWindowsClipboardFacade.h"
#include "inputleap/IClipboard.h"
#include "common/stdvector.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class IMSWindowsClipboardConverter;
class IMSWindowsClipboardFacade;

//! Microsoft windows clipboard implementation
class MSWindowsClipboard : public IClipboard {
public:
    MSWindowsClipboard(HWND window);
    MSWindowsClipboard(HWND window, IMSWindowsClipboardFacade &facade);
    virtual ~MSWindowsClipboard();

    //! Empty clipboard without ownership
    /*!
    Take ownership of the clipboard and clear all data from it.
    This must be called between a successful open() and close().
    Return false if the clipboard ownership could not be taken;
    the clipboard should not be emptied in this case.  Unlike
    empty(), isOwnedByBarrier() will return false when emptied
    this way.  This is useful when barrier wants to put data on
    clipboard but pretend (to itself) that some other app did it.
    When using empty(), barrier assumes the data came from the
    server and doesn't need to be sent back.  emptyUnowned()
    makes barrier send the data to the server.
    */
    bool emptyUnowned();

    //! Test if clipboard is owned by barrier
    static bool isOwnedByBarrier();

    // IClipboard overrides
    virtual bool empty();
    virtual void add(EFormat, const std::string& data);
    virtual bool open(Time) const;
    virtual void close() const;
    virtual Time getTime() const;
    virtual bool has(EFormat) const;
    virtual std::string get(EFormat) const;

    void setFacade(IMSWindowsClipboardFacade& facade);

private:
    void clearConverters();

    UINT convertFormatToWin32(EFormat) const;
    HANDLE convertTextToWin32(const std::string& data) const;
    std::string convertTextFromWin32(HANDLE) const;

    static UINT getOwnershipFormat();

private:
    typedef std::vector<IMSWindowsClipboardConverter*> ConverterList;

    HWND m_window;
    mutable Time m_time;
    ConverterList m_converters;
    static UINT            s_ownershipFormat;
    IMSWindowsClipboardFacade* m_facade;
    bool m_deleteFacade;
};

//! Clipboard format converter interface
/*!
This interface defines the methods common to all win32 clipboard format
converters.
*/
class IMSWindowsClipboardConverter {
public:
    virtual ~IMSWindowsClipboardConverter() { }
    // accessors

    // return the clipboard format this object converts from/to
    virtual IClipboard::EFormat
                        getFormat() const = 0;

    // return the atom representing the win32 clipboard format that
    // this object converts from/to
    virtual UINT getWin32Format() const = 0;

    // convert from the IClipboard format to the win32 clipboard format.
    // the input data must be in the IClipboard format returned by
    // getFormat().  the return data will be in the win32 clipboard
    // format returned by getWin32Format(), allocated by GlobalAlloc().
    virtual HANDLE fromIClipboard(const std::string&) const = 0;

    // convert from the win32 clipboard format to the IClipboard format
    // (i.e., the reverse of fromIClipboard()).
    virtual std::string toIClipboard(HANDLE data) const = 0;
};
