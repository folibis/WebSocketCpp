#ifdef WITH_OPENSSL
#include "CommunicationSslClient.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "Lock.h"

using namespace WebSocketCpp;

CommunicationSslClient::CommunicationSslClient(const std::string& cert, const std::string& key) noexcept
    :

      ICommunicationClient(SocketPool::Domain::Inet,
          SocketPool::Type::Stream,
          SocketPool::Options::ReuseAddr | SocketPool::Options::Ssl)
{
    m_sockets.SetSslCredentials(cert, key);
    m_sockets.SetPort(DEFAULT_SSL_PORT);
}

CommunicationSslClient::~CommunicationSslClient()
{
    Close();
}

bool CommunicationSslClient::Init()
{
    signal(SIGPIPE, SIG_IGN);
    if (m_initialized == true)
    {
        SetLastError("already initialized");
        return true;
    }

    m_initialized = ICommunicationClient::Init();

    return m_initialized;
}

#endif
