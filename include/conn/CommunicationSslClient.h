#ifdef WITH_OPENSSL
#ifndef WEB_SOCKET_CPP_COMMUNICATIONSSLCLIENT_H
#define WEB_SOCKET_CPP_COMMUNICATIONSSLCLIENT_H

#include <openssl/err.h>
#include <openssl/ssl.h>

#include "ClientSocket.h"
#include "CommunicationClientBase.h"

namespace WebSocketCpp
{

class CommunicationSslClient : public CommunicationClientBase
{
public:
    CommunicationSslClient(const std::string& cert, const std::string& key) noexcept;
    ~CommunicationSslClient() override;

    CommunicationSslClient(const CommunicationSslClient&)            = delete;
    CommunicationSslClient& operator=(const CommunicationSslClient&) = delete;

    void        SetPort(int port) override;
    int         GetPort() const override;
    void        SetHost(const std::string& host) override;
    std::string GetHost() const override;

    bool Init() override;
    bool Connect(const std::string& host = "", int port = 0) override;
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;

    bool Write(const ByteArray& data) override;

    bool SetDataReadyCallback(DataReadyCallback callback) override;
    bool SetCloseConnectionCallback(CloseConnectionCallback callback) override;

private:
    ClientSocket            m_client;
    std::string             m_host{};
    int                     m_port{443};
    std::string             m_cert;
    std::string             m_key;
    DataReadyCallback       m_data_cb;
    CloseConnectionCallback m_close_cb;
};

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_COMMUNICATIONSSLCLIENT_H
#endif // WITH_OPENSSL
