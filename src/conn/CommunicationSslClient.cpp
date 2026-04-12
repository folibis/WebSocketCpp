#ifdef WITH_OPENSSL
#include "CommunicationSslClient.h"

using namespace WebSocketCpp;

CommunicationSslClient::CommunicationSslClient(const std::string& cert, const std::string& key) noexcept
    : m_cert(cert),
      m_key(key)
{
    m_client.SetSslCredentials(cert, key);
}

CommunicationSslClient::~CommunicationSslClient()
{
    CommunicationSslClient::Close();
}

void CommunicationSslClient::SetPort(int port)
{
    m_port = port;
}

int CommunicationSslClient::GetPort() const
{
    return m_port;
}

void CommunicationSslClient::SetHost(const std::string& host)
{
    m_host = host;
}

std::string CommunicationSslClient::GetHost() const
{
    return m_host;
}

bool CommunicationSslClient::SetDataReadyCallback(DataReadyCallback callback)
{
    m_data_cb = std::move(callback);
    return true;
}

bool CommunicationSslClient::SetCloseConnectionCallback(CloseConnectionCallback callback)
{
    m_close_cb = std::move(callback);
    return true;
}

bool CommunicationSslClient::Init()
{
    if (IsInitialized())
    {
        return true;
    }

    if (!m_client.Init())
    {
        SetLastError(m_client.GetLastError());
        return false;
    }

    setInitialized(true);
    return true;
}

bool CommunicationSslClient::Connect(const std::string& host, int port)
{
    const std::string& h = host.empty() ? m_host : host;
    int                p = (port <= 0) ? m_port : port;

    m_client.SetOnData([this](ByteArray&& data) {
        if (m_data_cb)
        {
            m_data_cb(std::move(data));
        }
    });

    m_client.SetOnClose([this]() {
        if (m_close_cb)
        {
            m_close_cb();
        }
    });

    if (!m_client.Connect(h, p))
    {
        SetLastError(m_client.GetLastError());
        return false;
    }

    setConnected(true);
    return true;
}

bool CommunicationSslClient::Run()
{
    if (!m_client.Run())
    {
        SetLastError(m_client.GetLastError());
        return false;
    }

    setRunning(true);
    return true;
}

bool CommunicationSslClient::Close(bool wait)
{
    if (IsRunning() || IsConnected())
    {
        m_client.Close(wait);
        setRunning(false);
        setConnected(false);
    }
    return true;
}

bool CommunicationSslClient::WaitFor()
{
    return m_client.WaitFor();
}

bool CommunicationSslClient::Write(const ByteArray& data)
{
    if (!m_client.Write(data.data(), data.size()))
    {
        SetLastError(m_client.GetLastError());
        return false;
    }
    return true;
}

#endif // WITH_OPENSSL
