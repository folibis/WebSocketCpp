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
 *  */

#ifndef WEB_SOCKET_CPP_ICOMMUNICATION_SERVER_H
#define WEB_SOCKET_CPP_ICOMMUNICATION_SERVER_H

#include <functional>

#include "ICommunication.h"
#include "common.h"

namespace WebSocketCpp
{

class CommunicationServerBase : public ICommunication
{
public:
    virtual ~CommunicationServerBase() = default;

    using NewConnectionCallback   = std::function<void(int, const std::string&)>;
    using DataReadyCallback       = std::function<void(int, ByteArray)>;
    using CloseConnectionCallback = std::function<void(int)>;

    virtual bool Write(int connID, ByteArray& data)              = 0;
    virtual bool Write(int connID, ByteArray& data, size_t size) = 0;
    virtual bool CloseConnection(int connID)                     = 0;

    virtual bool SetNewConnectionCallback(NewConnectionCallback callback)   = 0;
    virtual bool SetDataReadyCallback(DataReadyCallback callback)           = 0;
    virtual bool SetCloseConnectionCallback(CloseConnectionCallback callback) = 0;

    bool IsConnected() const override
    {
        return m_connected;
    }

protected:
    void setConnected(bool connected)
    {
        m_connected = connected;
    }

private:
    bool m_connected = false;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_ICOMMUNICATION_SERVER_H
