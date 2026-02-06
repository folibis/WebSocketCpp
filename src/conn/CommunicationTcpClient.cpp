#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "CommunicationTcpClient.h"

#define DEFAULT_HTTP_PORT 80


using namespace WebSocketCpp;

CommunicationTcpClient::CommunicationTcpClient():
    CommunicationClientBase(SocketPool::Domain::Inet,
                         SocketPool::Type::Stream,
                         SocketPool::Options::ReuseAddr)
{
    m_socket.SetPort(DEFAULT_HTTP_PORT);

}

CommunicationTcpClient::~CommunicationTcpClient()
{
    Close();
}

bool CommunicationTcpClient::Init()
{
    if(m_initialized == true)
    {
        SetLastError("already initialized");
        return true;
    }

    m_initialized = CommunicationClientBase::Init();
    return m_initialized;
}

