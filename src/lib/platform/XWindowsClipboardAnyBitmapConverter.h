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

#pragma once

#include "platform/XWindowsClipboard.h"

//! Convert to/from some text encoding
class XWindowsClipboardAnyBitmapConverter :
                public IXWindowsClipboardConverter {
public:
    XWindowsClipboardAnyBitmapConverter();
    ~XWindowsClipboardAnyBitmapConverter() override;

    // IXWindowsClipboardConverter overrides
    IClipboard::EFormat getFormat() const override;
    int getDataSize() const override;
    std::string fromIClipboard(const std::string&) const override;
    std::string toIClipboard(const std::string&) const override;

protected:
    //! Convert from IClipboard format
    /*!
    Convert raw BGR pixel data to another image format.
    */
    virtual std::string doBGRFromIClipboard(const UInt8* bgrData, std::uint32_t w,
                                            std::uint32_t h) const = 0;

    //! Convert from IClipboard format
    /*!
    Convert raw BGRA pixel data to another image format.
    */
    virtual std::string doBGRAFromIClipboard(const UInt8* bgrData, std::uint32_t w,
                                             std::uint32_t h) const = 0;

    //! Convert to IClipboard format
    /*!
    Convert an image into raw BGR or BGRA image data and store the
    width, height, and image depth (24 or 32).
    */
    virtual std::string doToIClipboard(const std::string&, std::uint32_t& w, std::uint32_t& h,
                                       std::uint32_t& depth) const = 0;
};
