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

#ifndef REQUESTWEBSOCKET_H
#define REQUESTWEBSOCKET_H

#include "CommunicationClientBase.h"
#include "common.h"
#include "common_ws.h"

namespace WebSocketCpp
{

class RequestWebSocket
{
public:
    RequestWebSocket();
    RequestWebSocket(const RequestWebSocket&)                = delete;
    RequestWebSocket& operator=(const RequestWebSocket&)     = delete;
    RequestWebSocket(RequestWebSocket&&) noexcept            = default;
    RequestWebSocket& operator=(RequestWebSocket&&) noexcept = default;

    bool             Parse(const ByteArray& data);
    bool             IsFinal() const;
    MessageType      GetType() const;
    void             SetType(MessageType type);
    size_t           GetSize() const;
    const ByteArray& GetData() const;
    void             SetData(const ByteArray& data);

    bool Send(CommunicationClientBase* communication) const;

private:
    constexpr static uint64_t MAX_WEBSOCKET_MESSAGE_SIZE = 10 * 1024 * 1024; // 10Mb

    ByteArray   m_data;
    bool        m_final       = false;
    size_t      m_size        = 0;
    MessageType m_messageType = MessageType::Undefined;
};

} // namespace WebSocketCpp

#endif // REQUESTWEBSOCKET_H
