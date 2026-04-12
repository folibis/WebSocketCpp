#ifndef WEB_SOCKET_CPP_SERVER_SOCKET_H
#define WEB_SOCKET_CPP_SERVER_SOCKET_H

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#ifdef WITH_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

#include "IErrorable.h"
#include "IRunnable.h"
#include "MemoryPool.h"
#include "common.h"

namespace WebSocketCpp
{

class ServerSocket : public IErrorable, public IRunnable
{
public:
    ServerSocket(size_t client_count = MAX_CLIENT_COUNT);

    bool Init() override;
    bool Run() override;
    bool Close(bool wait) override;
    bool WaitFor() override;

    void SetAddress(const std::string& host, int32_t port);

    using OnConnectedCalback    = std::function<void(int32_t)>;
    using OnDisconnectedCalback = std::function<void(int32_t)>;
    using OnDataReadyCallback   = std::function<void(int32_t, ByteArray&&)>;

    void OnConnected(OnConnectedCalback callback);
    void OnDisconnected(OnDisconnectedCalback callback);
    void OnDataReady(OnDataReadyCallback callback);

    bool Write(int32_t idx, const uint8_t* data, size_t size);

#ifdef WITH_OPENSSL
    void SetSslCredentials(const std::string& cert, const std::string& key);
#endif

protected:
    bool    SetOptions(int32_t fd);
    bool    SetNonblocking(int32_t fd);
    void    ProcessTask();
    void    Cleanup();
    bool    AcceptClient();
    int32_t FindFreeConnection();
    bool    HandleRead(int32_t idx);
    bool    CloseSocket(int32_t fd);

    void OnConnect(int32_t idx);
    void OnDisconnect(int32_t idx);
    void OnData(int32_t idx, ByteArray&& data);
    void FreeData(const uint8_t* data);

    class Connection
    {
    public:
        Connection();
        ~Connection();
        int32_t GetFD() const;
        int32_t GetIdx() const;
        void    Reserve(int32_t fd);
        void    Assign(ServerSocket* server, int32_t fd, int32_t idx);
        void    Submit(const uint8_t* data, size_t size);
        void    Disconnect();
        void    Free();

    protected:
        void runThread();
        void processTask();

    private:
        enum class TaskType
        {
            UNDEFINED,
            CONNECTION,
            DISCONNECTION,
            DATA,
        };
        using TaskArg = std::tuple<TaskType, const uint8_t*, size_t>;

        ServerSocket*           m_server{nullptr};
        std::atomic<int32_t>    m_fd{-1};
        int32_t                 m_idx{-1};
        std::thread             m_process_thread;
        std::atomic_bool        m_running{false};
        std::condition_variable m_cv;
        std::mutex              m_args_mtx;
        std::queue<TaskArg>     m_args_queue;
    };

private:
#ifdef WITH_OPENSSL
    bool InitSsl();
    bool BeginSslHandshake(int32_t fd, int32_t idx);
    bool ContinueSslHandshake(int32_t idx);
#endif

    static constexpr size_t MAX_CLIENT_COUNT   = 100;
    static constexpr size_t LISTEN_QUEUE_SIZE  = 10;
    static constexpr size_t PROCESS_TIMEOUT_MS = 1000;
    static constexpr size_t BUFFER_SIZE        = 1024;
    static constexpr size_t MAX_EVENT_COUNT    = 64;

    std::mutex             m_write_mutex;
    size_t                 m_client_count{MAX_CLIENT_COUNT};
    std::string            m_host{};
    int32_t                m_port{-1};
    int32_t                m_server_fd{-1};
    int32_t                m_epoll_fd{-1};
    std::thread            m_process_thread;
    std::atomic<bool>      m_process_running{false};
    std::deque<Connection> m_connections;
    MemoryPool             m_memory_pool;
    OnConnectedCalback     m_connected_callback;
    OnDisconnectedCalback  m_disconnected_callback;
    OnDataReadyCallback    m_data_ready_callback;

#ifdef WITH_OPENSSL
    std::string                                        m_cert;
    std::string                                        m_key;
    SSL_CTX*                                           m_ssl_ctx{nullptr};
    std::unordered_map<int32_t, SSL*>                  m_ssl_conns;
    std::unordered_map<int32_t, std::pair<int32_t, SSL*>> m_pending_ssl;
#endif
};

} // namespace WebSocketCpp
#endif // WEB_SOCKET_CPP_SERVER_SOCKET_H
