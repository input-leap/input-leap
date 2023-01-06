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


#include "io/StreamBuffer.h"

//
// StreamBuffer
//

#include <cassert>

const std::uint32_t StreamBuffer::kChunkSize = 4096;

StreamBuffer::StreamBuffer() :
    m_size(0),
    m_headUsed(0)
{
    // do nothing
}

StreamBuffer::~StreamBuffer()
{
    // do nothing
}

const void* StreamBuffer::peek(std::uint32_t n)
{
    assert(n <= m_size);

    // if requesting no data then return nullptr so we don't try to access
    // an empty list.
    if (n == 0) {
        return nullptr;
    }

    // reserve space in first chunk
    ChunkList::iterator head = m_chunks.begin();
    head->reserve(n + m_headUsed);

    // consolidate chunks into the first chunk until it has n bytes
    ChunkList::iterator scan = head;
    ++scan;
    while (head->size() - m_headUsed < n && scan != m_chunks.end()) {
        head->insert(head->end(), scan->begin(), scan->end());
        scan = m_chunks.erase(scan);
    }

    return static_cast<const void*>(&(head->begin()[m_headUsed]));
}

void StreamBuffer::pop(std::uint32_t n)
{
    // discard all chunks if n is greater than or equal to m_size
    if (n >= m_size) {
        m_size     = 0;
        m_headUsed = 0;
        m_chunks.clear();
        return;
    }

    // update size
    m_size -= n;

    // discard chunks until more than n bytes would've been discarded
    ChunkList::iterator scan = m_chunks.begin();
    assert(scan != m_chunks.end());
    while (scan->size() - m_headUsed <= n) {
        n -= static_cast<std::uint32_t>(scan->size()) - m_headUsed;
        m_headUsed = 0;
        scan       = m_chunks.erase(scan);
        assert(scan != m_chunks.end());
    }

    // remove left over bytes from the head chunk
    if (n > 0) {
        m_headUsed += n;
    }
}

void StreamBuffer::write(const void* vdata, std::uint32_t n)
{
    assert(vdata != nullptr);

    // ignore if no data, otherwise update size
    if (n == 0) {
        return;
    }
    m_size += n;

    // cast data to bytes
    const std::uint8_t* data = static_cast<const std::uint8_t*>(vdata);

    // point to last chunk if it has space, otherwise append an empty chunk
    ChunkList::iterator scan = m_chunks.end();
    if (scan != m_chunks.begin()) {
        --scan;
        if (scan->size() >= kChunkSize) {
            ++scan;
        }
    }
    if (scan == m_chunks.end()) {
        scan = m_chunks.insert(scan, Chunk());
    }

    // append data in chunks
    while (n > 0) {
        // choose number of bytes for next chunk
        assert(scan->size() <= kChunkSize);
        std::uint32_t count = kChunkSize - static_cast<std::uint32_t>(scan->size());
        if (count > n)
            count = n;

        // transfer data
        scan->insert(scan->end(), data, data + count);
        n    -= count;
        data += count;

        // append another empty chunk if we're not done yet
        if (n > 0) {
            ++scan;
            scan = m_chunks.insert(scan, Chunk());
        }
    }
}

std::uint32_t StreamBuffer::getSize() const
{
    return m_size;
}
