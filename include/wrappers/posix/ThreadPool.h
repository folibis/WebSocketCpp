/*
 * Copyright (c) 2026 ruslan@muhlinin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WEB_SOCKET_CPP_THREAD_POOL_H
#define WEB_SOCKET_CPP_THREAD_POOL_H

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace WebSocketCpp
{

template <typename... Args>
class ThreadPool
{
public:
    using Task = std::function<void(Args...)>;

    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency(), size_t timeout_ms = 100)
        : m_running(false),
          m_threadCount(threads),
          m_timeoutMs(timeout_ms)
    {
    }

    ~ThreadPool()
    {
        Stop();
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    bool Init(Task task)
    {
        if (m_running)
        {
            return false;
        }
        m_task = std::move(task);
        return true;
    }

    bool Run()
    {
        if (m_running)
        {
            return false;
        }

        m_running = true;
        for (size_t i = 0; i < m_threadCount; i++)
        {
            m_workers.emplace_back([this]() { WorkerThread(); });
        }
        return true;
    }

    void Stop()
    {
        m_running = false;
        m_cv.notify_all();
        for (auto& w : m_workers)
        {
            if (w.joinable())
            {
                w.join();
            }
        }
        m_workers.clear();
    }

    template <typename... UArgs>
    void Submit(UArgs&&... args)
    {
        if (!m_running)
            return;
        {
            std::lock_guard<std::mutex> lock(m_mtx);
            m_queue.push(std::make_tuple(std::forward<UArgs>(args)...));
            m_cv.notify_one();
        }
    }

    bool IsRunning() const
    {
        return m_running;
    }

    size_t ThreadCount() const
    {
        return m_threadCount;
    }

    size_t QueueSize()
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_queue.size();
    }

protected:
    using ArgsTuple = std::tuple<Args...>;

    template<size_t... I>
    struct IndexSequence {};

    template<size_t N, size_t... I>
    struct MakeIndexSequence : MakeIndexSequence<N-1, N-1, I...> {};

    template<size_t... I>
    struct MakeIndexSequence<0, I...> : IndexSequence<I...> {};

    template<size_t... I>
    void ExecuteTask(ArgsTuple& args, IndexSequence<I...>)
    {
        m_task(std::get<I>(std::move(args))...);
    }

    void Execute(ArgsTuple& args)
    {
        ExecuteTask(args, MakeIndexSequence<sizeof...(Args)>{});
    }

    void WorkerThread()
    {
        while (m_running)
        {
            ArgsTuple args;
            bool      has_task = false;
            {
                std::unique_lock<std::mutex> lock(m_mtx);
                if (m_queue.empty())
                {
                    m_cv.wait_for(lock, std::chrono::milliseconds(m_timeoutMs));
                }
                if (!m_queue.empty())
                {
                    args     = std::move(m_queue.front());
                    has_task = true;
                    m_queue.pop();
                }
            }
            if (has_task)
            {
                Execute(args);
            }
        }
    }

private:
    std::vector<std::thread> m_workers;
    std::queue<ArgsTuple>   m_queue;
    std::mutex              m_mtx;
    std::condition_variable m_cv;
    Task                   m_task;
    std::atomic<bool>      m_running;
    size_t                 m_threadCount;
    size_t                 m_timeoutMs;
};

} // namespace WebSocketCpp
#endif // WEB_SOCKET_CPP_THREAD_POOL_H
