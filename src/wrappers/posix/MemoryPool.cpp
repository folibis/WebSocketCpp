#include "MemoryPool.h"

WebSocketCpp::MemoryPool::MemoryPool(std::size_t total_size)
    : m_total_size(total_size),
      m_used_size(0)
{
    m_buffer.resize(total_size);
    m_regions.reserve(DEFAULT_BLOCK_COUNT);
}

WebSocketCpp::MemoryPool::~MemoryPool()
{
}

uint8_t* WebSocketCpp::MemoryPool::allocate(std::size_t size)
{
    if (size == 0)
    {
        return nullptr;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    std::size_t cursor = 0;

    for (std::size_t i = 0; i < m_regions.size(); ++i)
    {
        if (m_regions[i].m_offset - cursor >= size)
        {
            m_regions.insert(m_regions.begin() + i, {cursor, size});
            m_used_size += size;
            return m_buffer.data() + cursor;
        }
        cursor = m_regions[i].m_offset + m_regions[i].m_size;
    }

    if (m_total_size - cursor >= size)
    {
        m_regions.push_back({cursor, size});
        m_used_size += size;
        return m_buffer.data() + cursor;
    }

    return nullptr;
}

void WebSocketCpp::MemoryPool::free(const uint8_t* ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    std::size_t offset = static_cast<std::size_t>(ptr - m_buffer.data());
    std::lock_guard<std::mutex> lock(m_mutex);

    for (std::size_t i = 0; i < m_regions.size(); ++i)
    {
        if (m_regions[i].m_offset != offset)
            continue;
        m_used_size -= m_regions[i].m_size;
        m_regions.erase(m_regions.begin() + i);
    }
}
