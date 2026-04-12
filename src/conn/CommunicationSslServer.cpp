#ifdef WITH_OPENSSL
#include "CommunicationSslServer.h"

using namespace WebSocketCpp;

CommunicationSslServer::CommunicationSslServer(const std::string& cert, const std::string& key) noexcept
    : m_cert(cert),
      m_key(key)
{
    m_server.SetSslCredentials(cert, key);
}

CommunicationSslServer::~CommunicationSslServer()
{
    CommunicationSslServer::Close();
}

void CommunicationSslServer::SetPort(int port)
{
    m_port = port;
}

int CommunicationSslServer::GetPort() const
{
    return m_port;
}

void CommunicationSslServer::SetHost(const std::string& host)
{
    m_host = host;
}

std::string CommunicationSslServer::GetHost() const
{
    return m_host;
}

bool CommunicationSslServer::Init()
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

bool CommunicationSslServer::Connect(const std::string& address, int port)
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

bool CommunicationSslServer::Run()
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

bool CommunicationSslServer::Close(bool wait)
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

bool CommunicationSslServer::WaitFor()
{
    return m_server.WaitFor();
}

bool CommunicationSslServer::Write(int connID, ByteArray& data)
{
    return Write(connID, data, data.size());
}

bool CommunicationSslServer::Write(int connID, ByteArray& data, size_t size)
{
    if (!m_server.Write(connID, data.data(), size))
    {
        SetLastError(m_server.GetLastError());
        return false;
    }
    return true;
}

bool CommunicationSslServer::CloseConnection(int /*connID*/)
{
    return true;
}

bool CommunicationSslServer::SetNewConnectionCallback(NewConnectionCallback callback)
{
    m_new_conn_cb = std::move(callback);
    return true;
}

bool CommunicationSslServer::SetDataReadyCallback(DataReadyCallback callback)
{
    m_data_cb = std::move(callback);
    return true;
}

bool CommunicationSslServer::SetCloseConnectionCallback(CloseConnectionCallback callback)
{
    m_close_cb = std::move(callback);
    return true;
}

#endif // WITH_OPENSSL
