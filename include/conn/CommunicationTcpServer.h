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

#ifndef WEB_SOCKET_CPP_COMMUNICATION_TCP_SERVER_H
#define WEB_SOCKET_CPP_COMMUNICATION_TCP_SERVER_H

#include "CommunicationServerBase.h"
#include "ServerSocket.h"

namespace WebSocketCpp
{

class CommunicationTcpServer : public CommunicationServerBase
{
public:
    explicit CommunicationTcpServer(size_t max_client_count = 100) noexcept;
    ~CommunicationTcpServer() override;

    CommunicationTcpServer(const CommunicationTcpServer&)            = delete;
    CommunicationTcpServer& operator=(const CommunicationTcpServer&) = delete;

    void        SetPort(int port) override;
    int         GetPort() const override;
    void        SetHost(const std::string& host) override;
    std::string GetHost() const override;

    bool Init() override;
    bool Connect(const std::string& address = "", int port = 0) override;
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;

    bool Write(int connID, ByteArray& data) override;
    bool Write(int connID, ByteArray& data, size_t size) override;
    bool CloseConnection(int connID) override;

    bool SetNewConnectionCallback(NewConnectionCallback callback) override;
    bool SetDataReadyCallback(DataReadyCallback callback) override;
    bool SetCloseConnectionCallback(CloseConnectionCallback callback) override;

private:
    ServerSocket            m_server;
    std::string             m_host{"*"};
    int                     m_port{80};
    NewConnectionCallback   m_new_conn_cb;
    DataReadyCallback       m_data_cb;
    CloseConnectionCallback m_close_cb;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_COMMUNICATION_TCP_SERVER_H
