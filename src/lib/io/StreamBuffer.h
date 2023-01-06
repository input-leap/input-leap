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

#include "base/EventTypes.h"
#include "common/stdlist.h"
#include "common/stdvector.h"

//! FIFO of bytes
/*!
This class maintains a FIFO (first-in, last-out) buffer of bytes.
*/
class StreamBuffer {
public:
    StreamBuffer();
    ~StreamBuffer();

    //! @name manipulators
    //@{

    //! Read data without removing from buffer
    /*!
    Return a pointer to memory with the next \c n bytes in the buffer
    (which must be <= getSize()).  The caller must not modify the returned
    memory nor delete it.
    */
    const void* peek(std::uint32_t n);

    //! Discard data
    /*!
    Discards the next \c n bytes.  If \c n >= getSize() then the buffer
    is cleared.
    */
    void pop(std::uint32_t n);

    //! Write data to buffer
    /*!
    Appends \c n bytes from \c data to the buffer.
    */
    void write(const void* data, std::uint32_t n);

    //@}
    //! @name accessors
    //@{

    //! Get size of buffer
    /*!
    Returns the number of bytes in the buffer.
    */
    std::uint32_t getSize() const;

    //@}

private:
    static const std::uint32_t kChunkSize;

    typedef std::vector<std::uint8_t> Chunk;
    typedef std::list<Chunk> ChunkList;

    ChunkList m_chunks;
    std::uint32_t m_size;
    std::uint32_t m_headUsed;
};
