#ifdef WITH_OPENSSL
#ifndef WEB_SOCKET_CPP_COMMUNICATION_SSL_SERVER_H
#define WEB_SOCKET_CPP_COMMUNICATION_SSL_SERVER_H

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "CommunicationServerBase.h"
#include "ServerSocket.h"

namespace WebSocketCpp
{

class CommunicationSslServer : public CommunicationServerBase
{
public:
    CommunicationSslServer(const std::string& cert, const std::string& key) noexcept;
    ~CommunicationSslServer() override;

    CommunicationSslServer(const CommunicationSslServer&)            = delete;
    CommunicationSslServer& operator=(const CommunicationSslServer&) = delete;

    void        SetPort(int port) override;
    int         GetPort() const override;
    void        SetHost(const std::string& host) override;
    std::string GetHost() const override;

    bool Init() override;
    bool Connect(const std::string& address = "", int port = 0) override;
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;

    bool Write(int connID, ByteArray& data) override;
    bool Write(int connID, ByteArray& data, size_t size) override;
    bool CloseConnection(int connID) override;

    bool SetNewConnectionCallback(NewConnectionCallback callback) override;
    bool SetDataReadyCallback(DataReadyCallback callback) override;
    bool SetCloseConnectionCallback(CloseConnectionCallback callback) override;

private:
    ServerSocket            m_server;
    std::string             m_host{"*"};
    int                     m_port{443};
    std::string             m_cert;
    std::string             m_key;
    NewConnectionCallback   m_new_conn_cb;
    DataReadyCallback       m_data_cb;
    CloseConnectionCallback m_close_cb;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_COMMUNICATION_SSL_SERVER_H
#endif // WITH_OPENSSL
