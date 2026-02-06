#ifdef WITH_OPENSSL
#include "CommunicationSslServer.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

using namespace WebSocketCpp;

CommunicationSslServer::CommunicationSslServer(const std::string& cert, const std::string& key) noexcept
    : CommunicationServerBase(SocketPool::Domain::Inet,
          SocketPool::Type::Stream,
          SocketPool::Options::ReuseAddr | SocketPool::Options::Ssl)
{
    m_sockets.SetSslCredentials(cert, key);
    m_sockets.SetPort(DEFAULT_SSL_PORT);
    m_sockets.SetHost(DEFAULT_SSL_HOST);
}

bool CommunicationSslServer::Init()
{
    ClearError();
    signal(SIGPIPE, SIG_IGN);

    if (m_initialized == true)
    {
        SetLastError("already initialized");
        return false;
    }

    m_initialized = CommunicationServerBase::Init();

    return m_initialized;
}

bool CommunicationSslServer::Connect(const std::string& address, int port)
{
    ClearError();

    if (m_initialized == false)
    {
        SetLastError("not initialized");
        return false;
    }

    m_connected = CommunicationServerBase::Connect(address, port);
    return m_connected;
}

#endif
