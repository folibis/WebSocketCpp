#include "CommunicationTcpServer.h"

using namespace WebSocketCpp;

CommunicationTcpServer::CommunicationTcpServer(size_t max_client_count) noexcept
    : m_server(max_client_count)
{
}

CommunicationTcpServer::~CommunicationTcpServer()
{
    CommunicationTcpServer::Close();
}

void CommunicationTcpServer::SetPort(int port)
{
    m_port = port;
}

int CommunicationTcpServer::GetPort() const
{
    return m_port;
}

void CommunicationTcpServer::SetHost(const std::string& host)
{
    m_host = host;
}

std::string CommunicationTcpServer::GetHost() const
{
    return m_host;
}

bool CommunicationTcpServer::Init()
{
    if (IsInitialized())
    {
        return true;
    }

    if (!m_server.Init())
    {
        SetLastError(m_server.GetLastError());
        return false;
    }

    setInitialized(true);
    return true;
}

bool CommunicationTcpServer::Connect(const std::string& address, int port)
{
    if (!address.empty())
    {
        m_host = address;
    }
    if (port > 0)
    {
        m_port = port;
    }
    setConnected(true);
    return true;
}

bool CommunicationTcpServer::Run()
{
    if (!IsInitialized())
    {
        SetLastError("not initialized");
        return false;
    }

    m_server.OnConnected([this](int32_t idx) {
        if (m_new_conn_cb)
        {
            m_new_conn_cb(idx, "");
        }
    });

    m_server.OnDisconnected([this](int32_t idx) {
        if (m_close_cb)
        {
            m_close_cb(idx);
        }
    });

    m_server.OnDataReady([this](int32_t idx, ByteArray&& data) {
        if (m_data_cb)
        {
            m_data_cb(idx, std::move(data));
        }
    });

    m_server.SetAddress(m_host, m_port);

    if (!m_server.Run())
    {
        SetLastError(m_server.GetLastError());
        return false;
    }

    setRunning(true);
    return true;
}

bool CommunicationTcpServer::Close(bool wait)
{
    if (IsRunning())
    {
        m_server.Close(wait);
        setRunning(false);
        setConnected(false);
        setInitialized(false);
    }
    return true;
}

bool CommunicationTcpServer::WaitFor()
{
    return m_server.WaitFor();
}

bool CommunicationTcpServer::Write(int connID, ByteArray& data)
{
    return Write(connID, data, data.size());
}

bool CommunicationTcpServer::Write(int connID, ByteArray& data, size_t size)
{
    if (!m_server.Write(connID, data.data(), size))
    {
        SetLastError(m_server.GetLastError());
        return false;
    }
    return true;
}

bool CommunicationTcpServer::CloseConnection(int /*connID*/)
{
    // ServerSocket manages connection lifecycle internally
    return true;
}

bool CommunicationTcpServer::SetNewConnectionCallback(NewConnectionCallback callback)
{
    m_new_conn_cb = std::move(callback);
    return true;
}

bool CommunicationTcpServer::SetDataReadyCallback(DataReadyCallback callback)
{
    m_data_cb = std::move(callback);
    return true;
}

bool CommunicationTcpServer::SetCloseConnectionCallback(CloseConnectionCallback callback)
{
    m_close_cb = std::move(callback);
    return true;
}
