#include "ClientSocket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "LogWriter.h"
#include "common.h"

#ifdef WITH_OPENSSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#endif

namespace WebSocketCpp
{

ClientSocket::ClientSocket()
{
}

ClientSocket::~ClientSocket()
{
    Shutdown();
    if (m_read_thread.joinable())
    {
        m_read_thread.join();
    }
    FreeSsl();
}

bool ClientSocket::Init()
{
    if (IsInitialized())
    {
        return true;
    }

    m_epoll_fd = epoll_create1(0);
    if (m_epoll_fd < 0)
    {
        SetLastError(std::string("epoll_create1 error: ") + strerror(errno));
        return false;
    }

    setInitialized(true);
    return true;
}

bool ClientSocket::Connect(const std::string& host, int32_t port)
{
    ClearError();

    if (!IsInitialized())
    {
        SetLastError("not initialized");
        return false;
    }

    if (m_connected)
    {
        SetLastError("already connected");
        return false;
    }

    struct addrinfo  hints{};
    struct addrinfo* result = nullptr;
    hints.ai_family         = AF_INET;
    hints.ai_socktype       = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result) != 0 || result == nullptr)
    {
        SetLastError(std::string("host resolution error: ") + strerror(errno));
        return false;
    }

    m_fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_fd < 0)
    {
        SetLastError(std::string("socket creation error: ") + strerror(errno));
        freeaddrinfo(result);
        return false;
    }

    if (!SetNonblocking(m_fd))
    {
        close(m_fd);
        m_fd = -1;
        freeaddrinfo(result);
        return false;
    }

    int ret = connect(m_fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    if (ret < 0 && errno != EINPROGRESS)
    {
        SetLastError(std::string("connect error: ") + strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }

    if (ret < 0)
    {
        if (!WaitConnect())
        {
            close(m_fd);
            m_fd = -1;
            return false;
        }
    }

    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLERR | EPOLLHUP;
    ev.data.fd = m_fd;
    if (epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_fd, &ev) < 0)
    {
        SetLastError(std::string("epoll_ctl error: ") + strerror(errno));
        close(m_fd);
        m_fd = -1;
        return false;
    }


#ifdef WITH_OPENSSL
    if (!m_cert.empty() || !m_key.empty())
    {
        if (!InitSsl())
        {
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_fd, nullptr);
            close(m_fd);
            m_fd = -1;
            return false;
        }
        if (!ConnectSsl())
        {
            epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_fd, nullptr);
            close(m_fd);
            m_fd = -1;
            return false;
        }
    }
#endif

    m_connected = true;
    return true;
}

bool ClientSocket::WaitConnect()
{
    epoll_event ev{};
    ev.events  = EPOLLOUT | EPOLLERR;
    ev.data.fd = m_fd;
    epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_fd, &ev);

    epoll_event event{};
    int         n = epoll_wait(m_epoll_fd, &event, 1, CONNECT_TIMEOUT_MS);

    epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_fd, nullptr);

    if (n == 0)
    {
        SetLastError("connect timeout");
        return false;
    }

    if (n < 0)
    {
        SetLastError(std::string("epoll_wait error: ") + strerror(errno));
        return false;
    }

    int       err = 0;
    socklen_t len = sizeof(err);
    getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, &len);
    if (err != 0)
    {
        SetLastError(std::string("connect error: ") + strerror(err));
        return false;
    }

    return true;
}

bool ClientSocket::Run()
{
    if (!IsInitialized())
    {
        SetLastError("not initialized");
        return false;
    }

    if (!m_connected)
    {
        SetLastError("not connected");
        return false;
    }

    if (IsRunning())
    {
        SetLastError("already running");
        return false;
    }

    m_read_running = true;
    m_read_thread  = std::thread(&ClientSocket::ReadLoop, this);
    setRunning(true);
    return true;
}

bool ClientSocket::Close(bool wait)
{
    Shutdown();
    WaitFor();
    FreeSsl();
    return true;
}

bool ClientSocket::WaitFor()
{
    if (m_read_thread.joinable())
    {
        m_read_thread.join();
    }
    return true;
}

void ClientSocket::Shutdown()
{
    m_connected    = false;
    m_read_running = false;

    if (m_fd >= 0)
    {
        epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, m_fd, nullptr);
        close(m_fd);
        m_fd = -1;
    }
}

void ClientSocket::FreeSsl()
{
#ifdef WITH_OPENSSL
    if (m_ssl)
    {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }
    if (m_ssl_ctx)
    {
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
    }
#endif
}

bool ClientSocket::Write(const uint8_t* data, size_t size)
{
    if (!m_connected || m_fd < 0)
    {
        SetLastError("not connected");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_write_mutex);

#ifdef WITH_OPENSSL
    if (m_ssl)
    {
        size_t total = 0;
        while (total < size)
        {
            int sent = SSL_write(m_ssl, data + total, static_cast<int>(size - total));
            if (sent <= 0)
            {
                int err = SSL_get_error(m_ssl, sent);
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
        ssize_t sent = send(m_fd, data + total, size - total, MSG_NOSIGNAL);
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

bool ClientSocket::IsConnected() const
{
    return m_connected;
}

void ClientSocket::SetOnData(OnDataCallback callback)
{
    m_data_callback = std::move(callback);
}

void ClientSocket::SetOnClose(OnCloseCallback callback)
{
    m_close_callback = std::move(callback);
}

bool ClientSocket::SetNonblocking(int32_t fd)
{
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK) == -1)
    {
        SetLastError(std::string("set nonblocking error: ") + strerror(errno));
        return false;
    }
    return true;
}

void ClientSocket::ReadLoop()
{
    epoll_event events[1];
    uint8_t     buffer[BUFFER_SIZE];

    while (m_read_running)
    {
        int n = epoll_wait(m_epoll_fd, events, 1, EPOLL_TIMEOUT_MS);

        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            LOG(std::string("epoll_wait error: ") + strerror(errno), LogWriter::LogType::Error);
            break;
        }

        if (n == 0)
        {
            continue;
        }

        uint32_t ev = events[0].events;

        if (ev & (EPOLLERR | EPOLLHUP))
        {
            HandleClose();
            break;
        }

        if (ev & EPOLLIN)
        {
            ssize_t size = 0;
#ifdef WITH_OPENSSL
            if (m_ssl)
            {
                size = SSL_read(m_ssl, buffer, static_cast<int>(BUFFER_SIZE));
                if (size <= 0)
                {
                    int err = SSL_get_error(m_ssl, static_cast<int>(size));
                    if (err != SSL_ERROR_WANT_READ)
                    {
                        HandleClose();
                        break;
                    }
                    continue;
                }
            }
            else
#endif
            {
                size = read(m_fd, buffer, BUFFER_SIZE);
                if (size == 0 || (size < 0 && errno != EAGAIN))
                {
                    HandleClose();
                    break;
                }
            }

            if (size > 0)
            {
                if (m_data_callback)
                {
                    m_data_callback(ByteArray(buffer, buffer + size));
                }
            }
        }
    }

    setRunning(false);
}

void ClientSocket::HandleClose()
{
    if (m_connected.exchange(false))
    {
        if (m_close_callback)
        {
            m_close_callback();
        }
    }
}

#ifdef WITH_OPENSSL
void ClientSocket::SetSslCredentials(const std::string& cert, const std::string& key)
{
    m_cert = cert;
    m_key  = key;
}

bool ClientSocket::InitSsl()
{
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    m_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ssl_ctx)
    {
        SetLastError("SSL_CTX_new failed");
        return false;
    }

    if (!m_cert.empty())
    {
        if (SSL_CTX_use_certificate_file(m_ssl_ctx, m_cert.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            SetLastError("SSL_CTX_use_certificate_file failed");
            SSL_CTX_free(m_ssl_ctx);
            m_ssl_ctx = nullptr;
            return false;
        }
    }

    if (!m_key.empty())
    {
        if (SSL_CTX_use_PrivateKey_file(m_ssl_ctx, m_key.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            SetLastError("SSL_CTX_use_PrivateKey_file failed");
            SSL_CTX_free(m_ssl_ctx);
            m_ssl_ctx = nullptr;
            return false;
        }
    }

    return true;
}

bool ClientSocket::ConnectSsl()
{
    m_ssl = SSL_new(m_ssl_ctx);
    if (!m_ssl)
    {
        SetLastError("SSL_new failed");
        return false;
    }

    SSL_set_fd(m_ssl, m_fd);

    while (true)
    {
        int ret = SSL_connect(m_ssl);
        if (ret == 1)
        {
            epoll_event ev{};
            ev.events  = EPOLLIN | EPOLLERR | EPOLLHUP;
            ev.data.fd = m_fd;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_fd, &ev);
            break;
        }

        int err = SSL_get_error(m_ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
        {
            uint32_t    want_ev = (err == SSL_ERROR_WANT_WRITE) ? (EPOLLOUT | EPOLLERR) : (EPOLLIN | EPOLLERR);
            epoll_event ev{};
            ev.events  = want_ev;
            ev.data.fd = m_fd;
            epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, m_fd, &ev);

            epoll_event event{};
            int         n = epoll_wait(m_epoll_fd, &event, 1, CONNECT_TIMEOUT_MS);
            if (n <= 0)
            {
                SetLastError(n == 0 ? "SSL_connect timeout" : "SSL_connect epoll_wait error");
                SSL_free(m_ssl);
                m_ssl = nullptr;
                SSL_CTX_free(m_ssl_ctx);
                m_ssl_ctx = nullptr;
                return false;
            }
            continue;
        }

        SetLastError("SSL_connect failed");
        SSL_free(m_ssl);
        m_ssl = nullptr;
        SSL_CTX_free(m_ssl_ctx);
        m_ssl_ctx = nullptr;
        return false;
    }

    return true;
}
#endif

} // namespace WebSocketCpp
