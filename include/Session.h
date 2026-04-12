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

#ifndef WEB_SOCKET_CPP_SESSION_H
#define WEB_SOCKET_CPP_SESSION_H

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <tuple>

#include "IDataHandler.h"
#include "MemoryPool.h"
#include "ThreadWorker.h"

namespace WebSocketCpp
{

class Session
{
public:
    Session(IDataHandler* handler, MemoryPool* pool);
    ~Session();

    Session(const Session&)            = delete;
    Session& operator=(const Session&) = delete;
    Session(Session&&)                 = delete;
    Session& operator=(Session&&)      = delete;

    void     Open(int clientID);
    void     Stop();
    bool     IsRunning() const;
    void     Submit(int clientID, const uint8_t* data, size_t size);
    uint8_t* getData(size_t size);

protected:
    void* Task(bool& running);

private:
    using ArgsTuple = std::tuple<int, const uint8_t*, size_t>;

    static constexpr uint64_t DEFAULT_WAIT_TIMEOUT_MS = 100;

    ThreadWorker            m_thread;
    IDataHandler*           m_handler = nullptr;
    MemoryPool*             m_pool    = nullptr;
    std::mutex              m_queueMutex;
    std::condition_variable m_signal;
    bool                    m_pending{false};
    std::queue<ArgsTuple>   m_queue;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_SESSION_H
