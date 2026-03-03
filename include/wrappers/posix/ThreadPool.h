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

#include <pthread.h>
#include <unistd.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <queue>
#include <vector>

#include "Lock.h"
#include "Mutex.h"
#include "Signal.h"

namespace WebSocketCpp
{

template <typename... Args>
class ThreadPool
{
public:
    using Task = std::function<void(Args...)>;

    explicit ThreadPool(size_t threads = static_cast<size_t>(sysconf(_SC_NPROCESSORS_ONLN)), size_t timeout_ms = 100)
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
        m_workers.resize(m_threadCount);
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
            if (pthread_create(&m_workers[i], nullptr, WorkerThread, this) != 0)
            {
                Stop();
                return false;
            }
        }
        return true;
    }

    void Stop()
    {
        m_running = false;
        m_cv.Fire();
        for (auto& w : m_workers)
        {
            pthread_join(w, nullptr);
        }
        m_workers.clear();
    }

    template <typename... UArgs>
    void Submit(UArgs&&... args)
    {
        if (!m_running)
            return;
        {
            Lock lock(m_mtx);
            m_queue.push(std::make_tuple(std::forward<UArgs>(args)...));
            m_cv.Fire();
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
        Lock lock(m_mtx);
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

    static void* WorkerThread(void* arg)
    {
        ThreadPool* pool = static_cast<ThreadPool*>(arg);
        while (pool->m_running)
        {
            ArgsTuple args;
            bool      has_task = false;
            {
                Lock lock(pool->m_mtx);
                if (pool->m_queue.empty())
                {
                    if(pool->m_cv.WaitTimeout(pool->m_mtx, pool->m_timeoutMs) == false)
                    {
                        continue;
                    }
                }

                if (!pool->m_queue.empty())
                {
                    args     = std::move(pool->m_queue.front());
                    has_task = true;
                    pool->m_queue.pop();
                }
            }
            if (has_task)
            {
                pool->Execute(args);
            }
        }
        return nullptr;
    }

private:
    std::vector<pthread_t> m_workers;
    std::queue<ArgsTuple>  m_queue;
    Mutex                  m_mtx;
    Signal                 m_cv;
    Task                   m_task;
    std::atomic<bool>      m_running;
    size_t                 m_threadCount;
    size_t                 m_timeoutMs;
};

} // namespace WebSocketCpp
#endif // WEB_SOCKET_CPP_THREAD_POOL_H
