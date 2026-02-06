#include "CommunicationClientBase.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

#include "DebugPrint.h"

using namespace WebSocketCpp;

CommunicationClientBase::CommunicationClientBase(SocketPool::Domain domain,
    SocketPool::Type                                                type,
    SocketPool::Options                                             options)
    : m_socket(1, SocketPool::Service::Client, domain, type, options)
{
}

void CommunicationClientBase::SetPort(int port)
{
    m_socket.SetPort(port);
}

int CommunicationClientBase::GetPort() const
{
    return m_socket.GetPort();
}

void CommunicationClientBase::SetHost(const std::string& host)
{
    m_socket.SetHost(host);
}

std::string CommunicationClientBase::GetHost() const
{
    return m_socket.GetHost();
}

bool CommunicationClientBase::Init()
{
    bool retval;
    ClearError();

    try
    {
        if (m_socket.Create(true) == ERROR)
        {
            SetLastError(std::string("client socket create error: ") + m_socket.GetLastError());
            throw std::runtime_error(GetLastError());
        }

        retval = true;
    }

    catch (...)
    {
        CloseConnection();
        DebugPrint() << "CommunicationClientBase::Init error: " << GetLastError() << std::endl;
        retval = false;
    }

    return retval;
}

bool CommunicationClientBase::Connect(const std::string& host, int port)
{
    ClearError();

    try
    {
        if (m_socket.Connect(host, port) == false)
        {
            SetLastError(std::string("socket connect error: ") + m_socket.GetLastError());
            throw std::runtime_error(GetLastError());
        }

        m_connected = true;
        return m_connected;
    }
    catch (...)
    {
        m_connected = false;
        CloseConnection();
        DebugPrint() << "CommunicationClientBase::Connect error: " << GetLastError() << std::endl;
        return false;
    }
}

bool CommunicationClientBase::CloseConnection()
{
    bool retval = false;
    if (m_socket.IsSocketValid(0))
    {
        retval = m_socket.CloseSocket(0);
        if (m_closeConnectionCallback != nullptr)
        {
            m_closeConnectionCallback();
        }
    }
    m_connected   = false;
    m_initialized = false;

    return retval;
}

bool CommunicationClientBase::Run()
{
    ClearError();

    if (m_initialized)
    {
        auto f = std::bind(&CommunicationClientBase::ReadThread, this, std::placeholders::_1);
        m_thread.SetFunction(f);
        m_running = m_thread.Start();
        if (m_running == false)
        {
            SetLastError(m_thread.GetLastError());
        }
    }

    return m_running;
}

bool CommunicationClientBase::Close(bool wait)
{
    if (m_initialized && m_connected)
    {
        CloseConnection();
    }

    if (m_running)
    {
        m_running = false;
        m_thread.Stop(wait);
    }

    return true;
}

bool CommunicationClientBase::WaitFor()
{
    if (m_running)
    {
        m_thread.Wait();
    }
    return true;
}

bool CommunicationClientBase::Write(const ByteArray& data)
{
    ClearError();
    bool retval = false;

    if (m_initialized == false || m_connected == false)
    {
        SetLastError("not initialized ot not connected");
        return false;
    }

    try
    {
        size_t sentBytes = m_socket.Write(data.data(), data.size());
        if (data.size() != sentBytes)
        {
            SetLastError(std::string("Send error: ") + strerror(errno), errno);
            throw GetLastError();
        }
        retval = true;
    }
    catch (...)
    {
        SetLastError("Write failed: " + GetLastError());
    }

    return retval;
}

ByteArray CommunicationClientBase::Read(size_t length)
{
    ClearError();
    bool      readMore = true;
    ByteArray data(BUFFER_SIZE);

    try
    {
        size_t readSize = length > BUFFER_SIZE ? BUFFER_SIZE : length;
        size_t all      = 0;
        size_t toRead;
        do
        {
            toRead = length - all;
            if (toRead > readSize)
            {
                toRead = readSize;
            }
            auto readBytes = m_socket.Read(&m_readBuffer, toRead);
            if (readBytes > 0)
            {
                data.insert(data.end(), m_readBuffer, m_readBuffer + readBytes);
                all += readBytes;
                if (all >= length)
                {
                    readMore = false;
                }
            }
            else
            {
                readMore = false;
            }
        } while (readMore);
    }
    catch (...)
    {
    }

    return data;
}

void* CommunicationClientBase::ReadThread(bool& running)
{
    ClearError();

    while (running)
    {
        m_socket.SetPollRead();
        try
        {
            if (m_socket.Poll())
            {
                if (m_socket.IsPollError(0))
                {
                    CloseConnection();
                }
                else
                {
                    auto readBytes = m_socket.Read(m_readBuffer, BUFFER_SIZE);
                    if (readBytes == ERROR)
                    {
                        CloseConnection();
                    }
                    else if (readBytes > 0)
                    {
                        if (m_dataReadyCallback != nullptr)
                        {
                            m_dataReadyCallback(ByteArray(m_readBuffer, m_readBuffer + readBytes));
                        }
                    }
                }
            }
        }
        catch (...)
        {
            SetLastError("read thread unexpected error");
        }
    }

    return nullptr;
}
