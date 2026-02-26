#include "WebSocketServer.h"

#include <cstring>

#include "CommunicationSslServer.h"
#include "CommunicationTcpServer.h"
#include "Data.h"
#include "FileSystem.h"
#include "Lock.h"
#include "LogWriter.h"
#include "common.h"
#include "common_ws.h"
#include "DebugPrint.h"

using namespace WebSocketCpp;

WebSocketServer::WebSocketServer()
    : m_config(WebSocketCpp::Config::Instance())
{
}

WebSocketServer::~WebSocketServer()
{
    WebSocketServer::Close();
}

bool WebSocketServer::Init()
{
    ClearError();

    m_protocol = m_config.GetWsProtocol();
    switch (m_protocol)
    {
        case Protocol::WS:
            m_server = std::unique_ptr<CommunicationServerBase>(new CommunicationTcpServer);
            break;
#ifdef WITH_OPENSSL
        case Protocol::WSS:
            m_server = std::unique_ptr<CommunicationServerBase>(new CommunicationSslServer(m_config.GetSslSertificate(), m_config.GetSslKey()));
            break;
#endif
        default:
            break;
    }

    if (m_server == nullptr)
    {
        SetLastError("protocol isn't set or not implemented");
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    m_server->SetPort(m_config.GetWsServerPort());
    if (!m_server->Init())
    {
        SetLastError("WebSocketServer init failed");
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    LOG(ToString(), LogWriter::LogType::Info);

    FileSystem::ChangeDir(FileSystem::GetApplicationFolder());

    auto f1 = std::bind(&WebSocketServer::ClientConnected, this, std::placeholders::_1, std::placeholders::_2);
    m_server->SetNewConnectionCallback(f1);
    auto f2 = std::bind(&WebSocketServer::DataReady, this, std::placeholders::_1, std::placeholders::_2);
    m_server->SetDataReadyCallback(f2);
    auto f3 = std::bind(&WebSocketServer::ClientDisconnected, this, std::placeholders::_1);
    m_server->SetCloseConnectionCallback(f3);

    if (StartRequestThread() == false)
    {
        return false;
    }

    setInitialized(true);

    return true;
}

bool WebSocketServer::Run()
{
    if (IsInitialized())
    {
        if (IsRunning() == false)
        {
            if (!m_server->Connect())
            {
                SetLastError("websocket server init failed: " + m_server->GetLastError());
                return false;
            }

            if (!m_server->Run())
            {
                SetLastError("websocket server run failed: " + m_server->GetLastError());
                return false;
            }

            setRunning(true);
        }
    }
    else
    {
        SetLastError("not initialized");
        return false;
    }


    return IsRunning();
}

bool WebSocketServer::Close(bool wait)
{
    if (IsRunning())
    {
        m_server->Close(wait);
        StopRequestThread();
        setRunning(false);
    }

    return true;
}

bool WebSocketServer::WaitFor()
{
    return m_server->WaitFor();
}

void WebSocketServer::OnMessage(const std::string& path, OnMessageCallback func)
{
    Lock lock(m_routeMutex);
    auto it = std::find_if(m_routes.begin(), m_routes.end(),
        [&path](const RouteWebSocket& r) { return r.GetPath() == path; });

    if (it != m_routes.end())
    {
        it->SetFunctionMessage(std::move(func));
        LOG("Updated route: " + it->ToString(), LogWriter::LogType::Info);
    }
    else
    {
        m_routes.emplace_back(path, func);
        LOG("Registered route: " + m_routes.back().ToString(), LogWriter::LogType::Info);
    }
}

void WebSocketServer::OnConnect(OnConnectCallback func)
{
    m_connect_callback = std::move(func);
}

void WebSocketServer::OnDisconnect(OnDisconnectCallback func)
{
    m_disconnect_callback = std::move(func);
}

bool WebSocketServer::SendResponse(const ResponseWebSocket& response)
{
    if (!response.IsEmpty())
    {
        return response.Send(m_server.get());
    }

    return false;
}

Protocol WebSocketServer::GetProtocol() const
{
    return m_protocol;
}

std::string WebSocketServer::ToString() const
{
    return m_config.ToString();
}

void WebSocketServer::ClientConnected(int connID, const std::string& remote)
{
    LOG(std::string("client connected: #") + std::to_string(connID) + ", " + remote, LogWriter::LogType::Access);
    InitConnection(connID, remote);
    if (m_connect_callback)
    {
        auto request_data = getRequest(connID);
        m_connect_callback(request_data->request);
    }
}

void WebSocketServer::DataReady(int connID, ByteArray data)
{
    PutToQueue(connID, std::move(data));
    SendSignal();
}

void WebSocketServer::ClientDisconnected(int connID)
{
    LOG(std::string("websocket connection closed: #") + std::to_string(connID), LogWriter::LogType::Access);

    if (m_disconnect_callback)
    {
        auto request_data = getRequest(connID);
        if (request_data != nullptr)
        {
            m_disconnect_callback(request_data->request);
        }
    }
    RemoveFromQueue(connID);
}

bool WebSocketServer::StartRequestThread()
{
    auto f = std::bind(&WebSocketServer::RequestThread, this, std::placeholders::_1);
    m_requestThread.SetFunction(f);

    if (m_requestThread.Start() == false)
    {
        SetLastError("failed to run request thread: " + m_requestThread.GetLastError());
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    return true;
}

bool WebSocketServer::StopRequestThread()
{
    if (m_requestThread.IsRunning())
    {
        m_requestThread.StopNoWait();
        SendSignal();
        m_requestThread.Wait();
    }
    return true;
}

void* WebSocketServer::RequestThread(bool& running)
{
    while (m_requestThread.IsRunning())
    {
        if(HasData() == false)
        {
            WaitForSignal();
        }
        if (CheckData())
        {
            ProcessRequests();
        }
    }

    return nullptr;
}

void WebSocketServer::SendSignal()
{
    Lock lock(m_signalMutex);
    m_signalCondition.Fire();
}

void WebSocketServer::WaitForSignal()
{
    Lock lock(m_signalMutex);
    m_signalCondition.Wait(m_signalMutex);
}

void WebSocketServer::PutToQueue(int connID, ByteArray&& data)
{
    Lock lock(m_queueMutex);
    for (auto& req : m_requestQueue)
    {
        if (req.connID == connID)
        {
            req.data.insert(req.data.end(), std::make_move_iterator(data.begin()), std::make_move_iterator(data.end()));
            break;
        }
    }
}

bool WebSocketServer::IsQueueEmpty()
{
    Lock lock(m_queueMutex);
    return m_requestQueue.empty();
}

void WebSocketServer::InitConnection(int connID, const std::string& remote)
{
    Lock lock(m_queueMutex);

    if (getRequest(connID) == nullptr)
    {
        m_requestQueue.emplace_back(connID, remote);
    }
}

bool WebSocketServer::CheckData()
{
    bool retval = false;
    Lock lock(m_queueMutex);

    for (RequestData& requestData : m_requestQueue)
    {
        if (requestData.readyForDispatch == false)
        {
            if (requestData.handshake == false)
            {
                retval |= CheckWsHeader(requestData);
            }
            else
            {
                bool frameParsed;
                while ((frameParsed = CheckWsFrame(requestData)))
                {
                    retval |= frameParsed;
                }
            }
        }
    }

    return retval;
}

bool WebSocketServer::CheckWsHeader(RequestData& requestData)
{
    bool retval = false;

    if (requestData.request.Parse(requestData.data))
    {
        size_t size = requestData.request.GetRequestSize();
        if (requestData.data.size() >= size)
        {
            requestData.request.SetMethod(Method::WEBSOCKET);
            requestData.data.erase(requestData.data.begin(), requestData.data.begin() + size);
            requestData.readyForDispatch = true;
            requestData.handshake        = false;
            retval                       = true;
        }
    }

    return retval;
}

bool WebSocketServer::CheckWsFrame(RequestData& requestData)
{
    bool retval = false;
    Lock lock(m_requestMutex);

    RequestWebSocket request;
    if (request.Parse(requestData.data))
    {
        size_t size = request.GetSize();
        requestData.data.erase(requestData.data.begin(), requestData.data.begin() + size);
        requestData.requestList.emplace_back(std::move(request));
        requestData.readyForDispatch = true;
        requestData.handshake        = true;
        retval                       = true;
    }

    return retval;
}

void WebSocketServer::ProcessRequests()
{
    Lock lock(m_queueMutex);

    for (auto& entry : m_requestQueue)
    {
        if (entry.readyForDispatch)
        {
            if (entry.handshake == false)
            {
                if (ProcessRequest(entry.request))
                {
                    entry.handshake        = true;
                    entry.readyForDispatch = false;
                }
            }
            else
            {
                Lock lock(m_requestMutex);
                if (entry.requestList.size() > 0)
                {
                    auto it = entry.requestList.begin();
                    while (it != entry.requestList.end())
                    {
                        if (it->IsFinal())
                        {
                            ProcessWsRequest(entry.request, *it);
                            it = entry.requestList.erase(it);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    entry.readyForDispatch = false;
                }
            }
        }
    }
}

bool WebSocketServer::HasData()
{
    for (auto& entry : m_requestQueue)
    {
        if(!entry.data.empty() && entry.handshake == true)
        {
            return true;
        }
    }

    return false;
}

void WebSocketServer::RemoveFromQueue(int connID)
{
    Lock lock(m_queueMutex);

    for (auto it = m_requestQueue.begin(); it != m_requestQueue.end(); ++it)
    {
        if (it->connID == connID)
        {
            m_requestQueue.erase(it);
            break;
        }
    }
}

bool WebSocketServer::ProcessRequest(Request& request)
{
    Response response(request.GetConnectionID(), m_config);
    bool     processed = false;
    bool     matched   = false;

    for (auto& route : m_routes)
    {
        if (route.IsMatch(request))
        {
            matched = true;
            auto& f = route.GetFunctionRequest();
            if (f != nullptr)
            {
                try
                {
                    if ((processed = f(request, response)))
                    {
                        break;
                    }
                }
                catch (...)
                {
                }
            }
        }
    }

    // the uri is matched but not request handler is provided or request is not processed
    if (processed == false && matched == true)
    {
        if (m_config.GetWsProcessDefault() == true)
        {
            std::string key = request.GetHeader().GetHeader("Sec-WebSocket-Key");
            key             = key + WEBSOCKET_KEY_TOKEN;

            auto buffer = Data::Sha1Digest(key);
            key         = Data::Base64Encode(buffer.data(), Data::SHA1_DIGEST_LENGTH);

            response.SetResponseCode(101);
            response.AddHeader(Header::HeaderType::Date, FileSystem::GetDateTime());
            response.AddHeader(Header::HeaderType::Upgrade, "websocket");
            response.AddHeader(Header::HeaderType::Connection, "upgrade");
            response.AddHeader("Sec-WebSocket-Accept", key);
            response.AddHeader("Sec-WebSocket-Version", WS_VERSION);
        }
        else
        {
            response.NotFound();
        }
    }

    return response.Send(m_server.get());
}

bool WebSocketServer::ProcessWsRequest(Request& request, const RequestWebSocket& wsRequest)
{
    ResponseWebSocket response(request.GetConnectionID());
    bool              processed = false;

    auto type = wsRequest.GetType();
    switch (type)
    {
        case MessageType::Text:
        case MessageType::Binary:
        {
            Lock lock(m_routeMutex);
            for (auto& route : m_routes)
            {
                if (route.IsMatch(request))
                {
                    auto& f = route.GetFunctionMessage();
                    if (f != nullptr)
                    {
                        try
                        {
                            if (f(request, response, wsRequest.GetData()) == true)
                            {
                                break;
                            }
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
        }
        break;
        case MessageType::Ping:
            response.WriteBinary(wsRequest.GetData());
            response.SetMessageType(MessageType::Pong);
            break;
        case MessageType::Close:

            break;
        default:
            break;
    }

    if (!response.IsEmpty())
    {
        response.Send(m_server.get());
    }

    return true;
}

RouteWebSocket* WebSocketServer::GetRoute(const std::string& path)
{
    for (size_t i = 0; i < m_routes.size(); i++)
    {
        if (m_routes.at(i).GetPath() == path)
        {
            return &(m_routes.at(i));
        }
    }

    return nullptr;
}

WebSocketServer::RequestData* WebSocketServer::getRequest(int connID)
{
    for (auto& req : m_requestQueue)
    {
        if (req.connID == connID)
        {
            return &req;
        }
    }

    return nullptr;
}
