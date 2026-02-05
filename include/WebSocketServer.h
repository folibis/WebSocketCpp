/*
 *  * Copyright (c) 2021 ruslan@muhlinin.com
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

#ifndef WEB_SOCKET_CPP_WEBSOCKETSERVER_H
#define WEB_SOCKET_CPP_WEBSOCKETSERVER_H

#include <deque>
#include <memory>

#include "Config.h"
#include "ICommunicationServer.h"
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

    void OnMessage(const std::string& path, const std::function<bool(const Request& request, ResponseWebSocket& response, const ByteArray& data)>& func);

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

        int                           connID;
        Request                       request;
        ByteArray                     data;
        std::vector<RequestWebSocket> requestList;
        bool                          handshake;
        bool                          readyForDispatch;
    };

    void OnConnected(int connID, const std::string& remote);
    void OnDataReady(int connID, ByteArray& data);
    void OnClosed(int connID);

    bool  StartRequestThread();
    bool  StopRequestThread();
    void* RequestThread(bool& running);

    void SendSignal();
    void WaitForSignal();
    void InitConnection(int connID, const std::string& remote);
    void PutToQueue(int connID, ByteArray& data);

    bool            IsQueueEmpty();
    bool            CheckData();
    void            ProcessRequests();
    void            RemoveFromQueue(int connID);
    bool            ProcessRequest(Request& request);
    bool            CheckWsHeader(RequestData& requestData);
    bool            CheckWsFrame(RequestData& requestData);
    bool            ProcessWsRequest(Request& request, const RequestWebSocket& wsRequest);
    RouteWebSocket* GetRoute(const std::string& path);

private:
    std::shared_ptr<ICommunicationServer> m_server   = nullptr;
    Protocol                              m_protocol = Protocol::Undefined;
    ThreadWorker                          m_requestThread;
    Mutex                                 m_queueMutex;
    Mutex                                 m_signalMutex;
    Mutex                                 m_requestMutex;
    Signal                                m_signalCondition;
    std::deque<RequestData>               m_requestQueue;
    Config&                               m_config;
    std::vector<RouteWebSocket>           m_routes;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_WEBSOCKETSERVER_H
