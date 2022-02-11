/*
 * InputLeap -- mouse and keyboard sharing utility
 * Copyright (C) 2012-2016 Symless Ltd.
 * Copyright (C) 2011 Chris Schoeneman
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

#include "arch/IArchString.h"
#include "arch/Arch.h"
#include "common/common.h"

#include <climits>
#include <cstring>
#include <cstdlib>
#include <cwchar>

//
// use C library non-reentrant multibyte conversion with mutex
//

IArchString::~IArchString()
{
}

int
IArchString::convStringWCToMB(char* dst,
                const wchar_t* src, UInt32 n, bool* errors)
{
    ptrdiff_t len = 0;
    std::mbstate_t state = { };

    bool dummyErrors;
    if (errors == NULL) {
        errors = &dummyErrors;
    }

    if (dst == NULL) {
        char dummy[MB_LEN_MAX];
        for (const wchar_t* scan = src; n > 0; ++scan, --n) {
            std::size_t mblen = std::wcrtomb(dummy, *scan, &state);
            if (mblen == static_cast<std::size_t>(-1)) {
                *errors = true;
                mblen   = 1;
            }
            len += mblen;
        }
        std::size_t mblen = std::wcrtomb(dummy, L'\0', &state);
        if (mblen != static_cast<std::size_t>(-1)) {
            len += mblen - 1;
        }
    }
    else {
        char* dst0 = dst;
        for (const wchar_t* scan = src; n > 0; ++scan, --n) {
            std::size_t mblen = std::wcrtomb(dst, *scan, &state);
            if (mblen == static_cast<std::size_t>(-1)) {
                *errors = true;
                *dst++  = '?';
            }
            else {
                dst    += mblen;
            }
        }
        std::size_t mblen = std::wcrtomb(dst, L'\0', &state);
        if (mblen != static_cast<std::size_t>(-1)) {
            // don't include nul terminator
            dst += mblen - 1;
        }
        len = dst - dst0;
    }

    return (int)len;
}

int
IArchString::convStringMBToWC(wchar_t* dst,
                const char* src, UInt32 n_param, bool* errors)
{
    std::mbstate_t state = { };

    ptrdiff_t n = (ptrdiff_t)n_param; // fix compiler warning
    ptrdiff_t len = 0;
    wchar_t dummy;

    bool dummyErrors;
    if (errors == NULL) {
        errors = &dummyErrors;
    }

    if (dst == NULL) {
        for (const char* scan = src; n > 0; ) {
            std::size_t mblen = std::mbrtowc(&dummy, scan, n, &state);
            switch (mblen) {
            case static_cast<std::size_t>(-2):
                // incomplete last character.  convert to unknown character.
                *errors = true;
                len    += 1;
                n       = 0;
                break;

            case static_cast<std::size_t>(-1):
                // invalid character.  count one unknown character and
                // start at the next byte.
                *errors = true;
                len    += 1;
                scan   += 1;
                n      -= 1;
                break;

            case 0:
                len    += 1;
                scan   += 1;
                n      -= 1;
                break;

            default:
                // normal character
                len    += 1;
                scan   += mblen;
                n      -= mblen;
                break;
            }
        }
    }
    else {
        wchar_t* dst0 = dst;
        for (const char* scan = src; n > 0; ++dst) {
            std::size_t mblen = std::mbrtowc(dst, scan, n, &state);
            switch (mblen) {
            case static_cast<std::size_t>(-2):
                // incomplete character.  convert to unknown character.
                *errors = true;
                *dst    = (wchar_t)0xfffd;
                n       = 0;
                break;

            case static_cast<std::size_t>(-1):
                // invalid character.  count one unknown character and
                // start at the next byte.
                *errors = true;
                *dst    = (wchar_t)0xfffd;
                scan   += 1;
                n      -= 1;
                break;

            case 0:
                *dst    = (wchar_t)0x0000;
                scan   += 1;
                n      -= 1;
                break;

            default:
                // normal character
                scan   += mblen;
                n      -= mblen;
                break;
            }
        }
        len = dst - dst0;
    }

    return (int)len;
}
