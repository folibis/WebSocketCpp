/*
 *  * Copyright (c) 2026 ruslan@muhlinin.com
 *  * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *  * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifndef WEB_SOCKET_CPP_MEMORY_POOL_H
#define WEB_SOCKET_CPP_MEMORY_POOL_H

#include <cstddef>
#include <mutex>
#include <vector>

namespace WebSocketCpp
{

class MemoryPool
{
public:
    explicit MemoryPool(std::size_t total_size);
    ~MemoryPool();
    MemoryPool(const MemoryPool&)            = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&)                 = delete;
    MemoryPool& operator=(MemoryPool&&)      = delete;

    uint8_t* allocate(std::size_t size);

    void free(const uint8_t* ptr);

    inline std::size_t total_size() const noexcept
    {
        return m_total_size;
    }

    inline std::size_t used_size() const noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        return m_used_size;
    }

    inline std::size_t free_size() const noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        return m_total_size - m_used_size;
    }

    inline std::size_t allocation_count() const noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        return m_regions.size();
    }

    inline bool empty() const noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        return m_used_size == 0;
    }

    inline bool full() const noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        return m_used_size == m_total_size;
    }

    void reset() noexcept
    {
        std::lock_guard<std::mutex> l(m_mutex);
        m_regions.clear();
        m_used_size = 0;
    }

private:
    static const size_t DEFAULT_BLOCK_COUNT = 10;
    struct Region
    {
        std::size_t m_offset;
        std::size_t m_size;
    };

    std::vector<uint8_t> m_buffer;
    std::vector<Region>  m_regions;

    const std::size_t  m_total_size;
    std::size_t        m_used_size{0};
    mutable std::mutex m_mutex;
};
} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_MEMORY_POOL_H
