#include "CommunicationTcpServer.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

#include "SocketPool.h"

#define DEFAULT_HTTP_PORT 80
#define DEFAULT_HTTP_HOST "*"

using namespace WebSocketCpp;

CommunicationTcpServer::CommunicationTcpServer() noexcept
    : ICommunicationServer(SocketPool::Domain::Inet,
          SocketPool::Type::Stream,
          SocketPool::Options::ReuseAddr)
{
    m_sockets.SetPort(DEFAULT_HTTP_PORT);
    m_sockets.SetHost(DEFAULT_HTTP_HOST);
}

CommunicationTcpServer::~CommunicationTcpServer()
{
    CommunicationTcpServer::Close();
}

bool CommunicationTcpServer::Init()
{
    if (m_initialized == true)
    {
        SetLastError("already initialized");
        return false;
    }

    m_initialized = ICommunicationServer::Init();
    return m_initialized;
}

bool CommunicationTcpServer::Connect(const std::string& address, int port)
{
    ClearError();

    if (m_initialized == false)
    {
        SetLastError("not initialized");
        return false;
    }

    m_connected = ICommunicationServer::Connect(address, port);
    return m_connected;
}
