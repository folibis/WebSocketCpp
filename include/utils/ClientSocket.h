#ifndef WEB_SOCKET_CPP_CLIENT_SOCKET_H
#define WEB_SOCKET_CPP_CLIENT_SOCKET_H

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "IErrorable.h"
#include "IRunnable.h"
#include "common.h"

#ifdef WITH_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

namespace WebSocketCpp
{

class ClientSocket : public IErrorable, public IRunnable
{
public:
    ClientSocket();
    ~ClientSocket();

    ClientSocket(const ClientSocket&)            = delete;
    ClientSocket& operator=(const ClientSocket&) = delete;
    ClientSocket(ClientSocket&&)                 = delete;
    ClientSocket& operator=(ClientSocket&&)      = delete;

    bool Init() override;
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;

    bool Connect(const std::string& host, int32_t port);
    bool Write(const uint8_t* data, size_t size);
    bool IsConnected() const;

    using OnDataCallback  = std::function<void(ByteArray&&)>;
    using OnCloseCallback = std::function<void()>;

    void SetOnData(OnDataCallback callback);
    void SetOnClose(OnCloseCallback callback);

#ifdef WITH_OPENSSL
    void SetSslCredentials(const std::string& cert, const std::string& key);
#endif

protected:
    bool SetNonblocking(int32_t fd);
    bool WaitConnect();
    void ReadLoop();
    void HandleClose();

private:
    void Shutdown();
    void FreeSsl();

#ifdef WITH_OPENSSL
    bool InitSsl();
    bool ConnectSsl();
#endif

private:
    static constexpr size_t BUFFER_SIZE        = 1024;
    static constexpr int    EPOLL_TIMEOUT_MS   = 500;
    static constexpr int    CONNECT_TIMEOUT_MS = 5000;

    int32_t           m_fd{-1};
    int32_t           m_epoll_fd{-1};
    std::thread       m_read_thread;
    std::atomic<bool> m_read_running{false};
    std::atomic<bool> m_connected{false};
    OnDataCallback    m_data_callback;
    OnCloseCallback   m_close_callback;
    std::mutex        m_write_mutex;

#ifdef WITH_OPENSSL
    std::string m_cert;
    std::string m_key;
    SSL_CTX*    m_ssl_ctx{nullptr};
    SSL*        m_ssl{nullptr};
#endif
};

} // namespace WebSocketCpp
#endif // WEB_SOCKET_CPP_CLIENT_SOCKET_H
