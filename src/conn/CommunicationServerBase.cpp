#include "CommunicationServerBase.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

#include "Lock.h"
#include "LogWriter.h"

using namespace WebSocketCpp;

CommunicationServerBase::CommunicationServerBase(
    size_t              max_client_count,
    SocketPool::Domain  domain,
    SocketPool::Type    type,
    SocketPool::Options options)
    : m_sockets(max_client_count + 1, SocketPool::Service::Server, domain, type, options),
      m_readBuffer(max_client_count, ByteArray(READ_BUFFER_SIZE)),
      m_processThreadPool(max_client_count)
{
}

void CommunicationServerBase::SetPort(int port)
{
    m_sockets.SetPort(port);
}

int CommunicationServerBase::GetPort() const
{
    return m_sockets.GetPort();
}

void CommunicationServerBase::SetHost(const std::string& host)
{
    m_sockets.SetHost(host);
}

std::string CommunicationServerBase::GetHost() const
{
    return m_sockets.GetHost();
}

bool CommunicationServerBase::Init()
{
    ClearError();
    bool retval;

    try
    {
        if (m_sockets.Create(true) == ERROR)
        {
            SetLastError(std::string("server socket create error: ") + m_sockets.GetLastError());
            throw std::runtime_error(GetLastError());
        }

        m_processThreadPool.Init([this](int clientID, const uint8_t* data, size_t size) {
            processTask(clientID, data, size);
        });

        setInitialized(true);
        retval = true;
    }

    catch (...)
    {
        CloseConnection(0);
        LOG("CommunicationServer::Init error: " + GetLastError(), LogWriter::LogType::Error);
        retval = false;
    }

    return retval;
}

bool CommunicationServerBase::Run()
{
    ClearError();

    try
    {
        auto f = std::bind(&CommunicationServerBase::ReadThread, this, std::placeholders::_1);
        m_readThread.SetFunction(f);
        setRunning(m_readThread.Start());
        m_processThreadPool.Run();
        if (IsRunning() == false)
        {
            SetLastError(m_readThread.GetLastError());
        }
    }
    catch (...)
    {
        setRunning(false);
    }

    return IsRunning();
}

bool CommunicationServerBase::WaitFor()
{
    m_readThread.Wait();
    return true;
}

bool CommunicationServerBase::Connect(const std::string& host, int port)
{
    ClearError();

    try
    {
        if (m_sockets.Bind(host, port) == false)
        {
            SetLastError(std::string("socket bind error: ") + m_sockets.GetLastError());
            throw std::runtime_error(GetLastError());
        }

        if (m_sockets.Listen() == false)
        {
            SetLastError(std::string("socket listen error: ") + m_sockets.GetLastError());
            throw std::runtime_error(GetLastError());
        }

        return true;
    }

    catch (...)
    {
        CloseConnection(0);
        LOG("CommunicationServer::Connect error: " + GetLastError(), LogWriter::LogType::Error);
        return false;
    }
}

bool CommunicationServerBase::Close(bool wait)
{
    if (IsRunning() == true)
    {
        m_readThread.Stop(true);
        setRunning(false);
        setInitialized(false);
        m_processThreadPool.Stop();
        CloseConnections();
    }

    return true;
}

bool CommunicationServerBase::CloseConnection(int connID)
{
    bool retval = m_sockets.CloseSocket(connID);
    if (retval)
    {
        if (m_closeConnectionCallback != nullptr)
        {
            m_closeConnectionCallback(connID);
        }
    }

    return retval;
}

void CommunicationServerBase::CloseConnections()
{
    m_sockets.CloseSockets();
}

bool CommunicationServerBase::Write(int connID, ByteArray& data)
{
    return Write(connID, data, data.size());
}

bool CommunicationServerBase::Write(int connID, ByteArray& data, size_t size)
{
    ClearError();

    if (IsInitialized() == false || IsConnected() == false)
    {
        SetLastError("not initialized or not connected");
        return false;
    }

    bool retval = false;
    Lock lock(m_writeMutex);

    try
    {
        auto pos = m_sockets.Write(data.data(), size, connID);
        retval   = (pos == size);
        if (retval == false)
        {
            SetLastError("send " + std::to_string(pos) + " of " + std::to_string(size) + " bytes");
        }
    }
    catch (const std::exception& ex)
    {
        SetLastError(std::string("CommunicationServer::Write() exception: ") + ex.what());
        retval = false;
    }

    return retval;
}

void* CommunicationServerBase::ReadThread(bool& running)
{
    int retval = (-1);

    try
    {
        m_sockets.SetPollRead();
        while (running)
        {
            if (m_sockets.Poll())
            {
                for (int i = 0; i < m_sockets.GetCount(); i++)
                {
                    if (m_sockets.IsPollError(i))
                    {
                        CloseConnection(i);
                    }
                    else if (m_sockets.HasData(i))
                    {
                        if (i == 0) // new client connected
                        {
                            int id = m_sockets.Accept();
                            if (id != ERROR)
                            {
                                if (m_newConnectionCallback != nullptr)
                                {
                                    m_processThreadPool.Submit(id, nullptr, 0);
                                }
                            }
                            else
                            {
                                LOG("socket #" + std::to_string(i) + " accept error: " + m_sockets.GetLastError(), LogWriter::LogType::Error);
                            }
                        }
                        else // existing socket data received
                        {
                            auto readBytes = m_sockets.Read(m_readBuffer[i - 1].data(), READ_BUFFER_SIZE, i);
                            if (readBytes != ERROR)
                            {
                                if (m_dataReadyCallback != nullptr)
                                {
                                    m_processThreadPool.Submit(i, m_readBuffer[i - 1].data(), readBytes);
                                }
                            }
                            else
                            {
                                CloseConnection(i);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        LOG("critical unexpected error occured in the read thread", LogWriter::LogType::Error);
    }

    return nullptr;
}

void CommunicationServerBase::processTask(int clientID, const uint8_t* data, size_t size)
{
    if (data == nullptr)
    {
        m_newConnectionCallback(clientID, m_sockets.GetRemoteAddress(clientID));
    }
    else
    {
        m_dataReadyCallback(clientID, ByteArray(data, data + size));
    }
}
