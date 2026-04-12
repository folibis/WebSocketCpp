#include "CommunicationTcpClient.h"

using namespace WebSocketCpp;

CommunicationTcpClient::CommunicationTcpClient() = default;

CommunicationTcpClient::~CommunicationTcpClient()
{
    CommunicationTcpClient::Close();
}

void CommunicationTcpClient::SetPort(int port)
{
    m_port = port;
}

int CommunicationTcpClient::GetPort() const
{
    return m_port;
}

void CommunicationTcpClient::SetHost(const std::string& host)
{
    m_host = host;
}

std::string CommunicationTcpClient::GetHost() const
{
    return m_host;
}

bool CommunicationTcpClient::SetDataReadyCallback(DataReadyCallback callback)
{
    m_data_cb = std::move(callback);
    return true;
}

bool CommunicationTcpClient::SetCloseConnectionCallback(CloseConnectionCallback callback)
{
    m_close_cb = std::move(callback);
    return true;
}

bool CommunicationTcpClient::Init()
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

bool CommunicationTcpClient::Connect(const std::string& host, int port)
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

bool CommunicationTcpClient::Run()
{
    if (!m_client.Run())
    {
        SetLastError(m_client.GetLastError());
        return false;
    }

    setRunning(true);
    return true;
}

bool CommunicationTcpClient::Close(bool wait)
{
    if (IsRunning() || IsConnected())
    {
        m_client.Close(wait);
        setRunning(false);
        setConnected(false);
    }
    return true;
}

bool CommunicationTcpClient::WaitFor()
{
    return m_client.WaitFor();
}

bool CommunicationTcpClient::Write(const ByteArray& data)
{
    if (!m_client.Write(data.data(), data.size()))
    {
        SetLastError(m_client.GetLastError());
        return false;
    }
    return true;
}
