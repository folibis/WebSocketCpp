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

CommunicationTcpServer::CommunicationTcpServer(size_t max_client_count) noexcept
    : CommunicationServerBase(
          max_client_count,
          SocketPool::Domain::Inet,
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
    if (IsInitialized() == true)
    {
        SetLastError("already initialized");
        return false;
    }

    setInitialized(CommunicationServerBase::Init());
    return IsInitialized();
}

bool CommunicationTcpServer::Connect(const std::string& address, int port)
{
    ClearError();

    if (IsInitialized() == false)
    {
        SetLastError("not initialized");
        return false;
    }

    setConnected(CommunicationServerBase::Connect(address, port));
    return IsConnected();
}
