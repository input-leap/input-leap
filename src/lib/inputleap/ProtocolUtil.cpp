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

#include "inputleap/ProtocolUtil.h"
#include "io/IStream.h"
#include "base/Log.h"
#include "inputleap/protocol_types.h"
#include "inputleap/XBarrier.h"
#include "common/stdvector.h"

#include <cctype>
#include <cstring>

//
// ProtocolUtil
//

void
ProtocolUtil::writef(inputleap::IStream* stream, const char* fmt, ...)
{
    assert(stream != NULL);
    assert(fmt != NULL);
    LOG((CLOG_DEBUG2 "writef(%s)", fmt));

    va_list args;
    va_start(args, fmt);
    std::uint32_t size = getLength(fmt, args);
    va_end(args);
    va_start(args, fmt);
    vwritef(stream, fmt, size, args);
    va_end(args);
}

bool
ProtocolUtil::readf(inputleap::IStream* stream, const char* fmt, ...)
{
    assert(stream != NULL);
    assert(fmt != NULL);
    LOG((CLOG_DEBUG2 "readf(%s)", fmt));

    bool result;
    va_list args;
    va_start(args, fmt);
    try {
        vreadf(stream, fmt, args);
        result = true;
    }
    catch (XIO&) {
        result = false;
    }
    va_end(args);
    return result;
}

void ProtocolUtil::vwritef(inputleap::IStream* stream, const char* fmt, std::uint32_t size,
                           va_list args)
{
    assert(stream != NULL);
    assert(fmt != NULL);

    // done if nothing to write
    if (size == 0) {
        return;
    }

    // fill buffer
    UInt8* buffer = new UInt8[size];
    writef_void(buffer, fmt, args);

    try {
        // write buffer
        stream->write(buffer, size);
        LOG((CLOG_DEBUG2 "wrote %d bytes", size));

        delete[] buffer;
    }
    catch (XBase&) {
        delete[] buffer;
        throw;
    }
}

void
ProtocolUtil::vreadf(inputleap::IStream* stream, const char* fmt, va_list args)
{
    assert(stream != NULL);
    assert(fmt != NULL);

    // begin scanning
    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            std::uint32_t len = eatLength(&fmt);
            switch (*fmt) {
            case 'i': {
                // check for valid length
                assert(len == 1 || len == 2 || len == 4);

                // read the data
                UInt8 buffer[4];
                read(stream, buffer, len);

                // convert it
                void* v = va_arg(args, void*);
                switch (len) {
                case 1:
                    // 1 byte integer
                    *static_cast<UInt8*>(v) = buffer[0];
                    LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len, *static_cast<UInt8*>(v), *static_cast<UInt8*>(v)));
                    break;

                case 2:
                    // 2 byte integer
                    *static_cast<std::uint16_t*>(v) =
                        static_cast<std::uint16_t>(
                        (static_cast<std::uint16_t>(buffer[0]) << 8) |
                         static_cast<std::uint16_t>(buffer[1]));
                    LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len,
                         *static_cast<std::uint16_t*>(v), *static_cast<std::uint16_t*>(v)));
                    break;

                case 4:
                    // 4 byte integer
                    *static_cast<std::uint32_t*>(v) =
                        (static_cast<std::uint32_t>(buffer[0]) << 24) |
                        (static_cast<std::uint32_t>(buffer[1]) << 16) |
                        (static_cast<std::uint32_t>(buffer[2]) <<  8) |
                         static_cast<std::uint32_t>(buffer[3]);
                    LOG((CLOG_DEBUG2 "readf: read %d byte integer: %d (0x%x)", len,
                         *static_cast<std::uint32_t*>(v), *static_cast<std::uint32_t*>(v)));
                    break;
                default:
                    break;
                }
                break;
            }

            case 'I': {
                // check for valid length
                assert(len == 1 || len == 2 || len == 4);

                // read the vector length
                UInt8 buffer[4];
                read(stream, buffer, 4);
                std::uint32_t n = (static_cast<std::uint32_t>(buffer[0]) << 24) |
                                  (static_cast<std::uint32_t>(buffer[1]) << 16) |
                                  (static_cast<std::uint32_t>(buffer[2]) <<  8) |
                                   static_cast<std::uint32_t>(buffer[3]);

                if (n > PROTOCOL_MAX_LIST_LENGTH) {
                    throw XBadClient("Too long message received");
                }

                // convert it
                void* v = va_arg(args, void*);
                switch (len) {
                case 1:
                    // 1 byte integer
                    for (std::uint32_t i = 0; i < n; ++i) {
                        read(stream, buffer, 1);
                        static_cast<std::vector<UInt8>*>(v)->push_back(
                            buffer[0]);
                        LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i, static_cast<std::vector<UInt8>*>(v)->back(), static_cast<std::vector<UInt8>*>(v)->back()));
                    }
                    break;

                case 2:
                    // 2 byte integer
                    for (std::uint32_t i = 0; i < n; ++i) {
                        read(stream, buffer, 2);
                        static_cast<std::vector<std::uint16_t>*>(v)->push_back(
                            static_cast<std::uint16_t>(
                            (static_cast<std::uint16_t>(buffer[0]) << 8) |
                             static_cast<std::uint16_t>(buffer[1])));
                        LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i,
                             static_cast<std::vector<std::uint16_t>*>(v)->back(),
                             static_cast<std::vector<std::uint16_t>*>(v)->back()));
                    }
                    break;

                case 4:
                    // 4 byte integer
                    for (std::uint32_t i = 0; i < n; ++i) {
                        read(stream, buffer, 4);
                        static_cast<std::vector<std::uint32_t>*>(v)->push_back(
                            (static_cast<std::uint32_t>(buffer[0]) << 24) |
                            (static_cast<std::uint32_t>(buffer[1]) << 16) |
                            (static_cast<std::uint32_t>(buffer[2]) <<  8) |
                             static_cast<std::uint32_t>(buffer[3]));
                        LOG((CLOG_DEBUG2 "readf: read %d byte integer[%d]: %d (0x%x)", len, i,
                             static_cast<std::vector<std::uint32_t>*>(v)->back(),
                             static_cast<std::vector<std::uint32_t>*>(v)->back()));
                    }
                    break;
                default:
                    break;
                }
                break;
            }

            case 's': {
                assert(len == 0);

                // read the string length
                UInt8 buffer[128];
                read(stream, buffer, 4);
                std::uint32_t str_len = (static_cast<std::uint32_t>(buffer[0]) << 24) |
                                        (static_cast<std::uint32_t>(buffer[1]) << 16) |
                                        (static_cast<std::uint32_t>(buffer[2]) <<  8) |
                                         static_cast<std::uint32_t>(buffer[3]);

                if (str_len > PROTOCOL_MAX_STRING_LENGTH) {
                    throw XBadClient("Too long message received");
                }

                // use a fixed size buffer if its big enough
                const bool useFixed = (str_len <= sizeof(buffer));

                // allocate a buffer to read the data
                UInt8* sBuffer = buffer;
                if (!useFixed) {
                    sBuffer = new UInt8[str_len];
                }

                // read the data
                try {
                    read(stream, sBuffer, str_len);
                }
                catch (...) {
                    if (!useFixed) {
                        delete[] sBuffer;
                    }
                    throw;
                }

                LOG((CLOG_DEBUG2 "readf: read %d byte string", str_len));

                // save the data
                std::string* dst = va_arg(args, std::string*);
                dst->assign(reinterpret_cast<const char*>(sBuffer), str_len);

                // release the buffer
                if (!useFixed) {
                    delete[] sBuffer;
                }
                break;
            }

            case '%':
                assert(len == 0);
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // next format character
            ++fmt;
        }
        else {
            // read next character
            char buffer[1];
            read(stream, buffer, 1);

            // verify match
            if (buffer[0] != *fmt) {
                LOG((CLOG_DEBUG2 "readf: format mismatch: %c vs %c", *fmt, buffer[0]));
                throw XIOReadMismatch();
            }

            // next format character
            ++fmt;
        }
    }
}

std::uint32_t ProtocolUtil::getLength(const char* fmt, va_list args)
{
    std::uint32_t n = 0;
    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            std::uint32_t len = eatLength(&fmt);
            switch (*fmt) {
            case 'i':
                assert(len == 1 || len == 2 || len == 4);
                (void)va_arg(args, std::uint32_t);
                break;

            case 'I':
                assert(len == 1 || len == 2 || len == 4);
                switch (len) {
                case 1:
                    len = 4 + static_cast<std::uint32_t>(
                                (va_arg(args, std::vector<UInt8>*))->size());
                    break;

                case 2:
                    len = 4 + 2 * static_cast<std::uint32_t>(
                                (va_arg(args, std::vector<std::uint16_t>*))->size());
                    break;

                case 4:
                    len = 4 + 4 * static_cast<std::uint32_t>(
                                (va_arg(args, std::vector<std::uint32_t>*))->size());
                    break;
                default:
                    break;
                }
                break;

            case 's':
                assert(len == 0);
                len = 4 + static_cast<std::uint32_t>((va_arg(args, std::string*))->size());
                (void)va_arg(args, UInt8*);
                break;

            case 'S':
                assert(len == 0);
                len = 4 + va_arg(args, std::uint32_t);
                (void)va_arg(args, UInt8*);
                break;

            case '%':
                assert(len == 0);
                len = 1;
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // accumulate size
            n += len;
            ++fmt;
        }
        else {
            // regular character
            ++n;
            ++fmt;
        }
    }
    return n;
}

void
ProtocolUtil::writef_void(void* buffer, const char* fmt, va_list args)
{
    UInt8* dst = static_cast<UInt8*>(buffer);

    while (*fmt) {
        if (*fmt == '%') {
            // format specifier.  determine argument size.
            ++fmt;
            std::uint32_t len = eatLength(&fmt);
            switch (*fmt) {
            case 'i': {
                const std::uint32_t v = va_arg(args, std::uint32_t);
                switch (len) {
                case 1:
                    // 1 byte integer
                    *dst++ = static_cast<UInt8>(v & 0xff);
                    break;

                case 2:
                    // 2 byte integer
                    *dst++ = static_cast<UInt8>((v >> 8) & 0xff);
                    *dst++ = static_cast<UInt8>( v       & 0xff);
                    break;

                case 4:
                    // 4 byte integer
                    *dst++ = static_cast<UInt8>((v >> 24) & 0xff);
                    *dst++ = static_cast<UInt8>((v >> 16) & 0xff);
                    *dst++ = static_cast<UInt8>((v >>  8) & 0xff);
                    *dst++ = static_cast<UInt8>( v        & 0xff);
                    break;

                default:
                    assert(0 && "invalid integer format length");
                    return;
                }
                break;
            }

            case 'I': {
                switch (len) {
                case 1: {
                    // 1 byte integers
                    const std::vector<UInt8>* list =
                        va_arg(args, const std::vector<UInt8>*);
                    const std::uint32_t n = static_cast<std::uint32_t>(list->size());
                    *dst++ = static_cast<UInt8>((n >> 24) & 0xff);
                    *dst++ = static_cast<UInt8>((n >> 16) & 0xff);
                    *dst++ = static_cast<UInt8>((n >>  8) & 0xff);
                    *dst++ = static_cast<UInt8>( n        & 0xff);
                    for (std::uint32_t i = 0; i < n; ++i) {
                        *dst++ = (*list)[i];
                    }
                    break;
                }

                case 2: {
                    // 2 byte integers
                    const std::vector<std::uint16_t>* list =
                        va_arg(args, const std::vector<std::uint16_t>*);
                    const std::uint32_t n = static_cast<std::uint32_t>(list->size());
                    *dst++ = static_cast<UInt8>((n >> 24) & 0xff);
                    *dst++ = static_cast<UInt8>((n >> 16) & 0xff);
                    *dst++ = static_cast<UInt8>((n >>  8) & 0xff);
                    *dst++ = static_cast<UInt8>( n        & 0xff);
                    for (std::uint32_t i = 0; i < n; ++i) {
                        const std::uint16_t v = (*list)[i];
                        *dst++ = static_cast<UInt8>((v >> 8) & 0xff);
                        *dst++ = static_cast<UInt8>( v       & 0xff);
                    }
                    break;
                }

                case 4: {
                    // 4 byte integers
                    const std::vector<std::uint32_t>* list =
                        va_arg(args, const std::vector<std::uint32_t>*);
                    const std::uint32_t n = static_cast<std::uint32_t>(list->size());
                    *dst++ = static_cast<UInt8>((n >> 24) & 0xff);
                    *dst++ = static_cast<UInt8>((n >> 16) & 0xff);
                    *dst++ = static_cast<UInt8>((n >>  8) & 0xff);
                    *dst++ = static_cast<UInt8>( n        & 0xff);
                    for (std::uint32_t i = 0; i < n; ++i) {
                        const std::uint32_t v = (*list)[i];
                        *dst++ = static_cast<UInt8>((v >> 24) & 0xff);
                        *dst++ = static_cast<UInt8>((v >> 16) & 0xff);
                        *dst++ = static_cast<UInt8>((v >>  8) & 0xff);
                        *dst++ = static_cast<UInt8>( v        & 0xff);
                    }
                    break;
                }

                default:
                    assert(0 && "invalid integer vector format length");
                    return;
                }
                break;
            }

            case 's': {
                assert(len == 0);
                const std::string* src = va_arg(args, std::string*);
                const std::uint32_t str_len =
                        (src != NULL) ? static_cast<std::uint32_t>(src->size()) : 0;
                *dst++ = static_cast<UInt8>((str_len >> 24) & 0xff);
                *dst++ = static_cast<UInt8>((str_len >> 16) & 0xff);
                *dst++ = static_cast<UInt8>((str_len >>  8) & 0xff);
                *dst++ = static_cast<UInt8>(str_len & 0xff);
                if (str_len != 0) {
                    memcpy(dst, src->data(), str_len);
                    dst += str_len;
                }
                break;
            }

            case 'S': {
                assert(len == 0);
                const std::uint32_t str_len = va_arg(args, std::uint32_t);
                const UInt8* src = va_arg(args, UInt8*);
                *dst++ = static_cast<UInt8>((str_len >> 24) & 0xff);
                *dst++ = static_cast<UInt8>((str_len >> 16) & 0xff);
                *dst++ = static_cast<UInt8>((str_len >>  8) & 0xff);
                *dst++ = static_cast<UInt8>(str_len & 0xff);
                memcpy(dst, src, str_len);
                dst += str_len;
                break;
            }

            case '%':
                assert(len == 0);
                *dst++ = '%';
                break;

            default:
                assert(0 && "invalid format specifier");
            }

            // next format character
            ++fmt;
        }
        else {
            // copy regular character
            *dst++ = *fmt++;
        }
    }
}

std::uint32_t ProtocolUtil::eatLength(const char** pfmt)
{
    const char* fmt = *pfmt;
    std::uint32_t n = 0;
    for (;;) {
        std::uint32_t d;
        switch (*fmt) {
        case '0': d = 0; break;
        case '1': d = 1; break;
        case '2': d = 2; break;
        case '3': d = 3; break;
        case '4': d = 4; break;
        case '5': d = 5; break;
        case '6': d = 6; break;
        case '7': d = 7; break;
        case '8': d = 8; break;
        case '9': d = 9; break;
        default: *pfmt = fmt; return n;
        }
        n = 10 * n + d;
        ++fmt;
    }
}

void ProtocolUtil::read(inputleap::IStream* stream, void* vbuffer, std::uint32_t count)
{
    assert(stream != NULL);
    assert(vbuffer != NULL);

    UInt8* buffer = static_cast<UInt8*>(vbuffer);
    while (count > 0) {
        // read more
        std::uint32_t n = stream->read(buffer, count);

        // bail if stream has hungup
        if (n == 0) {
            LOG((CLOG_DEBUG2 "unexpected disconnect in readf(), %d bytes left", count));
            throw XIOEndOfStream();
        }

        // prepare for next read
        buffer += n;
        count  -= n;
    }
}


//
// XIOReadMismatch
//

std::string XIOReadMismatch::getWhat() const noexcept
{
    return format("XIOReadMismatch", "ProtocolUtil::readf() mismatch");
}
