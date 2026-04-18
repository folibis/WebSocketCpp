#include "ServerSocket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>

#include "LogWriter.h"
#include "common.h"

#ifdef WITH_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

namespace WebSocketCpp
{

ServerSocket::ServerSocket(size_t client_count)
    : m_client_count(client_count),
      m_memory_pool(m_client_count * BUFFER_SIZE * 2)
{
    m_connections.resize(m_client_count);
}

bool ServerSocket::Init()
{
    if (IsInitialized() == false)
    {
        m_server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (m_server_fd < 0)
        {
            SetLastError(std::string("server socket creation error: ") + strerror(errno));
            return false;
        }

        if (!SetOptions(m_server_fd))
        {
            close(m_server_fd);
            m_server_fd = -1;
            return false;
        }

        if (!SetNonblocking(m_server_fd))
        {
            close(m_server_fd);
            m_server_fd = -1;
            return false;
        }

        m_epoll_fd = epoll_create1(0);
        if (m_epoll_fd < 0)
        {
            SetLastError(std::string("epoll_create1 error: ") + strerror(errno));
            close(m_server_fd);
            m_server_fd = -1;
            return false;
        }

        epoll_event ev{};
        ev.events   = EPOLLIN;
        ev.data.u32 = UINT32_MAX;
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_server_fd, &ev);

#ifdef WITH_OPENSSL
        if (!m_cert.empty() && !m_key.empty())
        {
            if (!InitSsl())
            {
                close(m_epoll_fd);
                m_epoll_fd = -1;
                close(m_server_fd);
                m_server_fd = -1;
                return false;
            }
        }
#endif

        setInitialized(true);
    }

    return true;
}

bool ServerSocket::Run()
{
    if (IsInitialized() == false)
    {
        SetLastError("not initialized");
        return false;
    }

    if (IsRunning())
    {
        SetLastError("already running");
        return false;
    }

    if (m_port <= 0)
    {
        SetLastError("port not set");
        return false;
    }

    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port   = htons(m_port);
    if (m_host.empty() || m_host == "*")
    {
        server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else
    {
        server_sockaddr.sin_addr.s_addr = inet_addr(m_host.c_str());
    }

    if (bind(m_server_fd, (struct sockaddr*)&server_sockaddr, sizeof(server_sockaddr)) == ERROR)
    {
        SetLastError(std::string("socket bind error: ") + strerror(errno));
        return false;
    }

    if (listen(m_server_fd, LISTEN_QUEUE_SIZE) == ERROR)
    {
        SetLastError(std::string("socket listen error: ") + strerror(errno));
        return false;
    }

    m_process_running = true;
    m_process_thread  = std::thread(&ServerSocket::ProcessTask, this);
    setRunning(true);

    return true;
}

bool ServerSocket::Close(bool wait)
{
    m_process_running = false;
    if (wait)
    {
        Cleanup();
    }
    return true;
}

bool ServerSocket::WaitFor()
{
    Cleanup();
    return true;
}

void ServerSocket::Cleanup()
{
    if (m_process_thread.joinable())
    {
        m_process_thread.join();
    }

    for (auto& conn : m_connections)
    {
        CloseSocket(conn.GetFD());
        conn.Free();
    }

#ifdef WITH_OPENSSL
    for (auto& [fd, idx_ssl] : m_pending_ssl)
    {
        SSL_free(idx_ssl.second);
        close(fd);
    }
    m_pending_ssl.clear();

    if (m_ssl_ctx)
    {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
    }
#endif

    if (m_epoll_fd >= 0)
    {
        close(m_epoll_fd);
        m_epoll_fd = -1;
    }

    if (m_server_fd >= 0)
    {
        close(m_server_fd);
        m_server_fd = -1;
    }

    setInitialized(false);
}

void ServerSocket::SetAddress(const std::string& host, int32_t port)
{
    m_host = host;
    m_port = port;
}

void ServerSocket::OnConnected(OnConnectedCalback callback)
{
    m_connected_callback = std::move(callback);
}

void ServerSocket::OnDisconnected(OnDisconnectedCalback callback)
{
    m_disconnected_callback = std::move(callback);
}

void ServerSocket::OnDataReady(OnDataReadyCallback callback)
{
    m_data_ready_callback = std::move(callback);
}

bool ServerSocket::Write(int32_t idx, const uint8_t* data, size_t size)
{
    if (idx < 0 || static_cast<size_t>(idx) >= m_connections.size())
    {
        SetLastError("invalid connection index");
        return false;
    }

    int32_t fd = m_connections[idx].GetFD();
    if (fd < 0)
    {
        SetLastError("connection not active");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_write_mutex);

#ifdef WITH_OPENSSL
    auto it = m_ssl_conns.find(fd);
    if (it != m_ssl_conns.end())
    {
        size_t total = 0;
        while (total < size)
        {
            int sent = SSL_write(it->second, data + total, static_cast<int>(size - total));
            if (sent <= 0)
            {
                int err = SSL_get_error(it->second, sent);
                if (err == SSL_ERROR_WANT_WRITE)
                {
                    continue;
                }
                SetLastError("SSL_write error");
                return false;
            }
            total += static_cast<size_t>(sent);
        }
        return true;
    }
#endif

    size_t total = 0;
    while (total < size)
    {
        ssize_t sent = send(fd, data + total, size - total, MSG_NOSIGNAL);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                continue;
            }
            SetLastError(std::string("write error: ") + strerror(errno));
            return false;
        }
        total += static_cast<size_t>(sent);
    }

    return true;
}

bool ServerSocket::SetOptions(int32_t fd)
{
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        SetLastError(std::string("setting socket option error: ") + strerror(errno));
        return false;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
    {
        SetLastError(std::string("setting socket option error: ") + strerror(errno));
        return false;
    }

    return true;
}

bool ServerSocket::SetNonblocking(int32_t fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1)
    {
        SetLastError(std::string("setting socket nonblocking error: ") + strerror(errno));
        return false;
    }

    return true;
}

void ServerSocket::ProcessTask()
{
    epoll_event events[MAX_EVENT_COUNT];

    while (m_process_running)
    {
        int32_t n = epoll_wait(m_epoll_fd, events, m_client_count < MAX_EVENT_COUNT ? m_client_count : MAX_EVENT_COUNT, PROCESS_TIMEOUT_MS);
        for (int32_t i = 0; i < n; i++)
        {
            if (events[i].data.u32 == UINT32_MAX)
            {
                if (AcceptClient() == false)
                {
                    LOG("failed to accept incoming connection", LogWriter::LogType::Error);
                }
            }
            else
            {
                uint32_t ev  = events[i].events;
                int32_t  idx = static_cast<int32_t>(events[i].data.u32);
                if (idx >= 0)
                {
#ifdef WITH_OPENSSL
                    if (ev & (EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP))
                    {
                        int32_t fd = m_connections[idx].GetFD();
                        auto    ps = m_pending_ssl.find(fd);
                        if (ps != m_pending_ssl.end())
                        {
                            if ((ev & (EPOLLERR | EPOLLHUP)) || !ContinueSslHandshake(idx))
                            {
                                SSL_free(ps->second.second);
                                m_pending_ssl.erase(ps);
                                epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                                close(fd);
                                m_connections[idx].Free();
                            }
                            continue;
                        }
                    }
#endif
                    if (ev & EPOLLIN)
                    {
                        if (HandleRead(idx) == false)
                        {
                            CloseSocket(m_connections[idx].GetFD());
                            m_connections[idx].Disconnect();
                        }
                    }
                    else if (ev & (EPOLLERR | EPOLLHUP))
                    {
                        CloseSocket(m_connections[idx].GetFD());
                        m_connections[idx].Disconnect();
                    }
                }
            }
        }
    }
}

bool ServerSocket::AcceptClient()
{
    int fd = accept(m_server_fd, nullptr, nullptr);
    if (fd < 0)
    {
        return (errno == EAGAIN || errno == EWOULDBLOCK);
    }

    int32_t idx = FindFreeConnection();
    if (idx == -1)
    {
        close(fd);
        return false;
    }

    SetNonblocking(fd);

#ifdef WITH_OPENSSL
    if (m_ssl_ctx)
    {
        return BeginSslHandshake(fd, idx);
    }
#endif

    epoll_event ev{};
    ev.events   = EPOLLIN;
    ev.data.u32 = static_cast<uint32_t>(idx);
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
    m_connections[idx].Assign(this, fd, idx);
    return true;
}

int32_t ServerSocket::FindFreeConnection()
{
    for (size_t i = 0; i < m_connections.size(); i++)
    {
        if (m_connections[i].GetFD() == -1)
        {
            return i;
        }
    }

    return -1;
}

bool ServerSocket::HandleRead(int32_t idx)
{
    uint8_t* buffer = m_memory_pool.allocate(BUFFER_SIZE);
    if (buffer == nullptr)
    {
        SetLastError("memory pool exhausted");
        LOG(GetLastError(), LogWriter::LogType::Error);
        Close(false);
        return false;
    }

    int32_t conn_fd = m_connections[idx].GetFD();
    int     size    = 0;

#ifdef WITH_OPENSSL
    auto it = m_ssl_conns.find(conn_fd);
    if (it != m_ssl_conns.end())
    {
        size = SSL_read(it->second, buffer, static_cast<int>(BUFFER_SIZE));
        if (size <= 0)
        {
            int err = SSL_get_error(it->second, size);
            m_memory_pool.free(buffer);
            return (err == SSL_ERROR_WANT_READ);
        }
        m_connections[idx].Submit(buffer, static_cast<size_t>(size));
        return true;
    }
#endif

    size = read(conn_fd, buffer, BUFFER_SIZE);
    if (size > 0)
    {
        m_connections[idx].Submit(buffer, static_cast<size_t>(size));
    }
    else if (size == 0)
    {
        m_memory_pool.free(buffer);
        return false;
    }
    else
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            m_memory_pool.free(buffer);
            return false;
        }
    }

    return true;
}

bool ServerSocket::CloseSocket(int32_t fd)
{
    if (fd >= 0)
    {
#ifdef WITH_OPENSSL
        auto it = m_ssl_conns.find(fd);
        if (it != m_ssl_conns.end())
        {
            SSL_shutdown(it->second);
            SSL_free(it->second);
            m_ssl_conns.erase(it);
        }
#endif
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }

    return true;
}

#ifdef WITH_OPENSSL
void ServerSocket::SetSslCredentials(const std::string& cert, const std::string& key)
{
    m_cert = cert;
    m_key  = key;
}

bool ServerSocket::InitSsl()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    m_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!m_ssl_ctx)
    {
        SetLastError("SSL_CTX_new failed");
        return false;
    }

    if (SSL_CTX_use_certificate_file(m_ssl_ctx, m_cert.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        SetLastError("SSL_CTX_use_certificate_file failed");
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_key.c_str(), SSL_FILETYPE_PEM) <= 0)
    {
        SetLastError("SSL_CTX_use_PrivateKey_file failed");
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
        return false;
    }

    return true;
}

bool ServerSocket::BeginSslHandshake(int32_t fd, int32_t idx)
{
    SSL* ssl = SSL_new(m_ssl_ctx);
    if (!ssl)
    {
        SetLastError("SSL_new failed");
        close(fd);
        return false;
    }

    SSL_set_fd(ssl, fd);

    int ret = SSL_accept(ssl);
    if (ret == 1)
    {
        m_ssl_conns[fd] = ssl;
        epoll_event ev{};
        ev.events   = EPOLLIN;
        ev.data.u32 = static_cast<uint32_t>(idx);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
        m_connections[idx].Assign(this, fd, idx);
        return true;
    }

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
    {
        m_connections[idx].Reserve(fd);
        m_pending_ssl[fd] = {idx, ssl};
        epoll_event ev{};
        ev.events   = EPOLLIN | EPOLLERR | EPOLLHUP;
        ev.data.u32 = static_cast<uint32_t>(idx);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
        return true;
    }

    SSL_free(ssl);
    close(fd);
    SetLastError("SSL_accept failed");
    return false;
}

bool ServerSocket::ContinueSslHandshake(int32_t idx)
{
    int32_t fd  = m_connections[idx].GetFD();
    auto    it  = m_pending_ssl.find(fd);
    SSL*    ssl = it->second.second;

    int ret = SSL_accept(ssl);
    if (ret == 1)
    {
        m_ssl_conns[fd] = ssl;
        m_pending_ssl.erase(it);

        epoll_event ev{};
        ev.events   = EPOLLIN;
        ev.data.u32 = static_cast<uint32_t>(idx);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
        m_connections[idx].Assign(this, fd, idx);
        return true;
    }

    int err = SSL_get_error(ssl, ret);
    if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
    {
        uint32_t    want_ev = (err == SSL_ERROR_WANT_WRITE)
                                  ? (EPOLLOUT | EPOLLERR | EPOLLHUP)
                                  : (EPOLLIN | EPOLLERR | EPOLLHUP);
        epoll_event ev{};
        ev.events   = want_ev;
        ev.data.u32 = static_cast<uint32_t>(idx);
        epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
        return true;
    }

    SetLastError("SSL_accept failed");
    return false;
}
#endif

void ServerSocket::OnConnect(int32_t idx)
{
    if (m_connected_callback)
    {
        m_connected_callback(idx);
    }
}

void ServerSocket::OnDisconnect(int32_t idx)
{
    if (m_disconnected_callback)
    {
        m_disconnected_callback(idx);
    }
}

void ServerSocket::OnData(int32_t idx, ByteArray&& data)
{
    if (m_data_ready_callback)
    {
        m_data_ready_callback(idx, std::move(data));
    }
}

void ServerSocket::FreeData(const uint8_t* data)
{
    m_memory_pool.free(data);
}

// ---------------------------- Connection ----------------------------------

ServerSocket::Connection::Connection()
{
}

ServerSocket::Connection::~Connection()
{
    Free();
}

void ServerSocket::Connection::Reserve(int32_t fd)
{
    m_fd = fd;
}

void ServerSocket::Connection::Assign(ServerSocket* server, int32_t fd, int32_t idx)
{
    if (m_process_thread.joinable())
    {
        m_process_thread.join();
    }
    m_server = server;
    m_fd     = fd;
    m_idx    = idx;
    runThread();
    std::unique_lock<std::mutex> lock(m_args_mtx);
    m_args_queue.push(std::make_tuple(TaskType::CONNECTION, nullptr, 0));
    m_cv.notify_one();
}

void ServerSocket::Connection::Submit(const uint8_t* data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_args_mtx);
    m_args_queue.push(std::make_tuple(TaskType::DATA, data, size));
    m_cv.notify_one();
}

void ServerSocket::Connection::Free()
{
    {
        std::unique_lock<std::mutex> lock(m_args_mtx);
        m_fd      = -1;
        m_idx     = -1;
        m_running = false;
        m_cv.notify_one();
    }
    if (m_process_thread.joinable())
    {
        m_process_thread.join();
    }
}

void ServerSocket::Connection::Disconnect()
{
    std::unique_lock<std::mutex> lock(m_args_mtx);
    m_args_queue.push(std::make_tuple(TaskType::DISCONNECTION, nullptr, 0));
    m_cv.notify_one();
}

void ServerSocket::Connection::runThread()
{
    m_running        = true;
    m_process_thread = std::thread(&Connection::processTask, this);
}

void ServerSocket::Connection::processTask()
{
    while (m_running)
    {
        std::unique_lock<std::mutex> lock(m_args_mtx);
        m_cv.wait_for(lock, std::chrono::milliseconds(100), [this]() { return !m_args_queue.empty(); });

        if (!m_args_queue.empty())
        {
            TaskType       task_type = std::get<0>(m_args_queue.front());
            const uint8_t* data      = std::get<1>(m_args_queue.front());
            size_t         size      = std::get<2>(m_args_queue.front());
            int32_t        idx       = m_idx;
            m_args_queue.pop();
            lock.unlock();

            switch (task_type)
            {
                case TaskType::CONNECTION:
                    m_server->OnConnect(idx);
                    break;
                case TaskType::DISCONNECTION:
                    m_server->OnDisconnect(idx);
                    {
                        std::unique_lock<std::mutex> lock(m_args_mtx);
                        m_fd  = -1;
                        m_idx = -1;
                    }
                    m_running = false;
                    return;
                case TaskType::DATA:
                {
                    m_server->OnData(idx, ByteArray(data, data + size));
                    m_server->FreeData(data);
                }
                break;
                default:
                    break;
            }
        }
    }
}

int32_t ServerSocket::Connection::GetFD() const
{
    return m_fd;
}

int32_t ServerSocket::Connection::GetIdx() const
{
    return m_idx;
}

} // namespace WebSocketCpp
