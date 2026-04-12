#include "Session.h"

#include <chrono>
#include <mutex>

using namespace WebSocketCpp;

constexpr uint64_t Session::DEFAULT_WAIT_TIMEOUT_MS;

Session::Session(IDataHandler* handler, MemoryPool* pool)
    : m_handler(handler), m_pool(pool)
{
    m_thread.SetFunction([this](bool& running) -> void* {
        return Task(running);
    });
}

Session::~Session()
{
    Stop();
}

void Session::Open(int clientID)
{
    m_thread.Start();
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.emplace(clientID, nullptr, 0);
        m_pending = true;
    }
    m_signal.notify_one();
}

void Session::Stop()
{
    m_thread.Stop();

    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_queue.empty())
    {
        const uint8_t* data = std::get<1>(m_queue.front());
        if (data != nullptr)
        {
            m_pool->free(data);
        }
        m_queue.pop();
    }
}

bool Session::IsRunning() const
{
    return m_thread.IsRunning();
}

void Session::Submit(int clientID, const uint8_t* data, size_t size)
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.emplace(clientID, data, size);
        m_pending = true;
    }
    m_signal.notify_one();
}

uint8_t* Session::getData(size_t size)
{
    return m_pool->allocate(size);
}

void* Session::Task(bool& running)
{
    while (running)
    {
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_signal.wait_for(lock, std::chrono::milliseconds(DEFAULT_WAIT_TIMEOUT_MS),
                [this]() { return m_pending; });
            m_pending = false;
        }

        while (true)
        {
            ArgsTuple arg;
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                if (m_queue.empty())
                {
                    break;
                }
                arg = m_queue.front();
                m_queue.pop();
            }

            int            clientID = std::get<0>(arg);
            const uint8_t* data     = std::get<1>(arg);
            size_t         size     = std::get<2>(arg);

            if (data == nullptr)
            {
                m_handler->OnConnect(clientID);
            }
            else
            {
                m_handler->OnData(clientID, data, size);
            }
        }
    }

    return nullptr;
}
