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

#ifndef WEB_SOCKET_CPP_WEBSOCKETSERVER_H
#define WEB_SOCKET_CPP_WEBSOCKETSERVER_H

#include <list>
#include <memory>

#include "CommunicationServerBase.h"
#include "Config.h"
#include "IErrorable.h"
#include "IRunnable.h"
#include "Mutex.h"
#include "Request.h"
#include "RequestWebSocket.h"
#include "ResponseWebSocket.h"
#include "RouteWebSocket.h"
#include "Signal.h"
#include "ThreadWorker.h"

namespace WebSocketCpp
{

class WebSocketServer : public IErrorable, public IRunnable
{
public:
    WebSocketServer();
    virtual ~WebSocketServer();
    WebSocketServer(const WebSocketServer& other)            = delete;
    WebSocketServer& operator=(const WebSocketServer& other) = delete;
    WebSocketServer(WebSocketServer&& other)                 = delete;
    WebSocketServer& operator=(WebSocketServer&& other)      = delete;

    bool Init() override;
    bool Init(WebSocketCpp::Config config);
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;

    using OnMessageCallback    = std::function<bool(const Request& request, ResponseWebSocket& response, const ByteArray& data)>;
    using OnConnectCallback    = std::function<void(const Request&)>;
    using OnDisconnectCallback = std::function<void(const Request&)>;

    void OnMessage(const std::string& path, OnMessageCallback func);
    void OnConnect(OnConnectCallback func);
    void OnDisconnect(OnDisconnectCallback func);

    bool SendResponse(const ResponseWebSocket& response);

    Protocol    GetProtocol() const;
    std::string ToString() const;

protected:
    struct RequestData
    {
        RequestData(int connID, const std::string& remote)
        {
            this->connID     = connID;
            readyForDispatch = false;
            handshake        = false;
            request.SetConnectionID(connID);
            request.GetHeader().SetRemote(remote);
        }

        int                           connID{-1};
        Request                       request;
        ByteArray                     data{};
        std::vector<RequestWebSocket> requestList;
        bool                          handshake{false};
        bool                          readyForDispatch{false};

        RequestData(const RequestData&)            = delete;
        RequestData& operator=(const RequestData&) = delete;
        RequestData(RequestData&&) noexcept        = delete;
        RequestData& operator=(RequestData&&)      = delete;
    };

    void ClientConnected(int connID, const std::string& remote);
    void DataReady(int connID, ByteArray data);
    void ClientDisconnected(int connID);

    bool  StartRequestThread();
    bool  StopRequestThread();
    void* RequestThread(bool& running);

    void SendSignal();
    void WaitForSignal();
    void InitConnection(int connID, const std::string& remote);
    void PutToQueue(int connID, ByteArray&& data);

    bool            IsQueueEmpty();
    bool            CheckData();
    void            ProcessRequests();
    void            RemoveFromQueue(int connID);
    bool            ProcessRequest(Request& request);
    bool            CheckWsHeader(RequestData& requestData);
    bool            CheckWsFrame(RequestData& requestData);
    bool            ProcessWsRequest(Request& request, const RequestWebSocket& wsRequest);
    RouteWebSocket* GetRoute(const std::string& path);
    RequestData*    getRequest(int connID);

private:
    std::unique_ptr<CommunicationServerBase> m_server   = nullptr;
    Protocol                                 m_protocol = Protocol::Undefined;
    ThreadWorker                             m_requestThread;
    Mutex                                    m_queueMutex;
    Mutex                                    m_signalMutex;
    Mutex                                    m_requestMutex;
    Signal                                   m_signalCondition;
    std::list<RequestData>                   m_requestQueue;
    Config&                                  m_config;
    std::vector<RouteWebSocket>              m_routes;
    Mutex                                    m_routeMutex;
    OnConnectCallback                        m_connect_callback;
    OnDisconnectCallback                     m_disconnect_callback;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_WEBSOCKETSERVER_H
