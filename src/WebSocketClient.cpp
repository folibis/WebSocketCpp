#include "WebSocketClient.h"

#include <chrono>
#include <mutex>

#include "CommunicationSslClient.h"
#include "CommunicationTcpClient.h"
#include "Data.h"
#include "LogWriter.h"
#include "RequestWebSocket.h"
#include "Response.h"

using namespace WebSocketCpp;

WebSocketClient::WebSocketClient()
    : m_config(WebSocketCpp::Config::Instance())
{
}

WebSocketClient::~WebSocketClient()
{
    WebSocketClient::Close();
}

bool WebSocketClient::Init()
{
    m_data.reserve(1024);
    return true;
}

bool WebSocketClient::Run()
{
    return true;
}

bool WebSocketClient::Close(bool wait)
{
    return m_connection->Close(wait);
}

bool WebSocketClient::WaitFor()
{
    return m_connection->WaitFor();
}

bool WebSocketClient::Open(Request& request)
{
    ClearError();

    const Url& url = request.GetUrl();
    if (m_connection == nullptr)
    {
        if (InitConnection(url) == false)
        {
            SetLastError("init failed: " + GetLastError());
            LOG(GetLastError(), LogWriter::LogType::Error);
            return false;
        }
    }

    if (m_connection->IsConnected() == false)
    {
        if (m_connection->Connect(url.GetHost(), url.GetPort()) == false)
        {
            SetLastError("connection faied: " + m_connection->GetLastError());
            LOG(GetLastError(), LogWriter::LogType::Error);
            return false;
        }

        SetState(State::Connected);
    }

    if (m_connection->Run() == false)
    {
        SetLastError("read routine failed: " + m_connection->GetLastError());
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    auto& header = request.GetHeader();
    header.SetHeader(Header::HeaderType::Host, request.GetUrl().GetHost());
    header.SetHeader(Header::HeaderType::Upgrade, "websocket");
    header.SetHeader(Header::HeaderType::Connection, "Upgrade");
    m_key = Data::Base64Encode(StringUtil::GenerateRandomString(16));
    header.SetHeader("Sec-WebSocket-Key", m_key);
    header.SetHeader("Sec-WebSocket-Version", WS_VERSION);

    {
        std::lock_guard<std::mutex> lock(m_handshake_mtx);
        m_handshake_done = false;
    }
    SetState(State::Handshake);

    if (request.Send(m_connection) == false)
    {
        SetLastError("request sending error: " + request.GetLastError());
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_handshake_mtx);
        bool                         ok = m_handshake_cv.wait_for(lock,
                                    std::chrono::milliseconds(m_config.GetClientConnectTimeoutMs()),
                                    [this]() { return m_handshake_done; });
        if (!ok)
        {
            SetState(State::HandshakeFailed);
            SetLastError("handshake timeout");
            return false;
        }
    }

    return true;
}

bool WebSocketClient::Open(const std::string& address)
{
    ClearError();

    Request request;
    auto&   p_url = request.GetUrl();
    p_url.Parse(address);
    if (p_url.IsInitiaized())
    {
        request.SetMethod(Method::GET);
        return Open(request);
    }
    else
    {
        SetLastError("Url parsing error");
        LOG(GetLastError(), LogWriter::LogType::Error);
    }

    return false;
}

bool WebSocketClient::SendText(const ByteArray& data)
{
    RequestWebSocket request;
    request.SetType(MessageType::Text);
    request.SetData(data);
    return request.Send(m_connection.get());
}

bool WebSocketClient::SendText(const std::string& data)
{
    return SendText(StringUtil::String2ByteArray(data));
}

bool WebSocketClient::SendBinary(const ByteArray& data)
{
    RequestWebSocket request;
    request.SetType(MessageType::Binary);
    request.SetData(data);
    return request.Send(m_connection.get());
}

bool WebSocketClient::SendBinary(const std::string& data)
{
    return SendBinary(StringUtil::String2ByteArray(data));
}

bool WebSocketClient::SendPing()
{
    RequestWebSocket request;
    request.SetType(MessageType::Ping);
    return request.Send(m_connection.get());
}

void WebSocketClient::SetOnConnect(OnConnectCallback callback)
{
    m_connectCallback = std::move(callback);
}

void WebSocketClient::SetOnClose(OnCloseCallback callback)
{
    m_closeCallback = std::move(callback);
}

void WebSocketClient::SetOnError(OnErrorCallback callback)
{
    m_errorCallback = std::move(callback);
}

void WebSocketClient::SetOnMessage(OnMessageCallback callback)
{
    m_messageCallback = std::move(callback);
}

void WebSocketClient::SetProgressCallback(ProgressCallback callback)
{
    m_progressCallback = std::move(callback);
}

void WebSocketClient::SetOnStateChanged(OnStateChangedCallback callback)
{
    m_stateCallback = std::move(callback);
}

void WebSocketClient::OnDataReady(ByteArray&& data)
{
    std::lock_guard<std::mutex> lock(m_read_mtx);
    m_data.insert(m_data.end(), data.begin(), data.end());
    if (m_state == State::Handshake)
    {
        Response response(0, m_config);
        size_t   all, downloaded;
        if (response.Parse(m_data, &all, &downloaded))
        {
            if (response.GetResponseCode() == 101)
            {
                auto&       header = response.GetHeader();
                std::string h      = header.GetHeader(Header::HeaderType::Upgrade);
                StringUtil::ToLower(h);
                if (h == "websocket")
                {
                    h = header.GetHeader(Header::HeaderType::Connection);
                    StringUtil::ToLower(h);
                    if (h == "upgrade")
                    {
                        h                  = header.GetHeader("Sec-WebSocket-Accept");
                        std::string key    = m_key + WEBSOCKET_KEY_TOKEN;
                        auto        buffer = Data::Sha1Digest(key);
                        key                = Data::Base64Encode(buffer.data(), Data::SHA1_DIGEST_LENGTH);
                        if (h == key)
                        {
                            SetState(State::BinaryMessage);
                            {
                                std::lock_guard<std::mutex> lock(m_handshake_mtx);
                                m_handshake_done = true;
                            }
                            m_handshake_cv.notify_one();
                            if (m_connectCallback != nullptr)
                            {
                                m_connectCallback(true);
                            }
                            m_data.erase(m_data.begin(), m_data.begin() + response.GetResponseSize());
                            return;
                        }
                        else
                        {
                            SetLastError("incorrect response key");
                        }
                    }
                    else
                    {
                        SetLastError("incorrect response header");
                    }
                }
                else
                {
                    SetLastError("incorrect response header");
                }
            }
            else
            {
                SetLastError("incorrect response code");
            }
        }
        else
        {
            SetLastError("error while response parsing");
        }

        SetState(State::Closed);
        if (m_connectCallback != nullptr)
        {
            m_connectCallback(false);
        }

        Close(false);
    }
    else if (m_state == State::BinaryMessage)
    {
        while (true)
        {
            ResponseWebSocket response(0);
            if (response.Parse(m_data) == false)
            {
                break;
            }
            m_data.erase(m_data.begin(), m_data.begin() + response.GetSize());
            if (m_messageCallback != nullptr)
            {
                m_messageCallback(response);
            }
        }
    }
}

void WebSocketClient::OnClosed()
{
    SetState(State::Closed);

    if (m_closeCallback != nullptr)
    {
        m_closeCallback();
    }
}

bool WebSocketClient::InitConnection(const Url& url)
{
    if (m_connection != nullptr)
    {
        m_connection->Close();
    }

    switch (url.GetScheme())
    {
        case Url::Scheme::WS:
            m_connection = std::make_shared<CommunicationTcpClient>();
            break;
#ifdef WITH_OPENSSL
        case Url::Scheme::WSS:
            m_connection = std::make_shared<CommunicationSslClient>(m_config.GetSslSertificate(), m_config.GetSslKey());
            break;
#endif
        default:
            break;
    }

    if (m_connection == nullptr)
    {
        SetLastError("provided scheme is incorrect or not supported");
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    m_connection->SetHost(url.GetHost());
    m_connection->SetPort(url.GetPort());

    if (!m_connection->Init())
    {
        SetLastError("WebSocketClient init failed");
        LOG(GetLastError(), LogWriter::LogType::Error);
        return false;
    }

    SetState(State::Initialized);

    auto f1 = std::bind(&WebSocketClient::OnDataReady, this, std::placeholders::_1);
    m_connection->SetDataReadyCallback(f1);
    auto f2 = std::bind(&WebSocketClient::OnClosed, this);
    m_connection->SetCloseConnectionCallback(f2);

    return true;
}

void WebSocketClient::SetState(State state)
{
    m_state = state;
    if (m_stateCallback != nullptr)
    {
        m_stateCallback(m_state);
    }
}
