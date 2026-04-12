/*
 * Copyright (c) 2026 ruslan@muhlinin.com
 * MIT License
 *
 * Deep integration tests for ServerSocket and ClientSocket.
 */

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "ClientSocket.h"
#include "ServerSocket.h"

using namespace WebSocketCpp;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int FindFreePort()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    ::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    ::bind(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    socklen_t len = sizeof(addr);
    ::getsockname(fd, reinterpret_cast<struct sockaddr*>(&addr), &len);
    int port = ntohs(addr.sin_port);
    ::close(fd);
    return port;
}

// Wait up to timeout_ms for pred to become true, notified via cv.
template<typename Pred>
static bool WaitFor(std::mutex& mtx, std::condition_variable& cv,
                    Pred pred, int timeout_ms = 2000)
{
    std::unique_lock<std::mutex> lock(mtx);
    return cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), pred);
}

// Small delay to let async events propagate.
static void Yield() { std::this_thread::sleep_for(std::chrono::milliseconds(50)); }

// ---------------------------------------------------------------------------
// ServerSocket — unit tests
// ---------------------------------------------------------------------------

TEST(ServerSocketUnit, RunWithoutPortFails)
{
    ServerSocket server;
    ASSERT_TRUE(server.Init());
    // No SetAddress called — port is -1
    EXPECT_FALSE(server.Run());
    EXPECT_FALSE(server.GetLastError().empty());
}

TEST(ServerSocketUnit, InitIdempotent)
{
    ServerSocket server;
    server.SetAddress("127.0.0.1", FindFreePort());
    EXPECT_TRUE(server.Init());
    EXPECT_TRUE(server.Init()); // second call must also succeed
}

TEST(ServerSocketUnit, DoubleRunFails)
{
    ServerSocket server;
    server.SetAddress("127.0.0.1", FindFreePort());
    ASSERT_TRUE(server.Init());
    ASSERT_TRUE(server.Run());
    EXPECT_FALSE(server.Run());
    EXPECT_FALSE(server.GetLastError().empty());
    server.Close(true);
}

TEST(ServerSocketUnit, WriteInvalidIndexFails)
{
    ServerSocket server;
    server.SetAddress("127.0.0.1", FindFreePort());
    ASSERT_TRUE(server.Init());
    ASSERT_TRUE(server.Run());

    uint8_t buf[] = {1, 2, 3};
    EXPECT_FALSE(server.Write(-1, buf, sizeof(buf)));
    EXPECT_FALSE(server.Write(999, buf, sizeof(buf)));
    server.Close(true);
}

// ---------------------------------------------------------------------------
// ClientSocket — unit tests
// ---------------------------------------------------------------------------

TEST(ClientSocketUnit, RunBeforeConnectFails)
{
    ClientSocket client;
    ASSERT_TRUE(client.Init());
    EXPECT_FALSE(client.Run());
    EXPECT_FALSE(client.GetLastError().empty());
}

TEST(ClientSocketUnit, ConnectToClosedPortFails)
{
    int port = FindFreePort(); // nothing listening
    ClientSocket client;
    ASSERT_TRUE(client.Init());
    EXPECT_FALSE(client.Connect("127.0.0.1", port));
    EXPECT_FALSE(client.GetLastError().empty());
}

TEST(ClientSocketUnit, ConnectBadHostFails)
{
    ClientSocket client;
    ASSERT_TRUE(client.Init());
    EXPECT_FALSE(client.Connect("this.host.does.not.exist.invalid", 9999));
    EXPECT_FALSE(client.GetLastError().empty());
}

TEST(ClientSocketUnit, WriteBeforeConnectFails)
{
    ClientSocket client;
    ASSERT_TRUE(client.Init());
    uint8_t buf[] = {1, 2, 3};
    EXPECT_FALSE(client.Write(buf, sizeof(buf)));
    EXPECT_FALSE(client.GetLastError().empty());
}

TEST(ClientSocketUnit, CloseBeforeConnectDoesNotCrash)
{
    ClientSocket client;
    ASSERT_TRUE(client.Init());
    EXPECT_TRUE(client.Close(true)); // must not crash
}

TEST(ClientSocketUnit, DoubleConnectFails)
{
    int port = FindFreePort();
    ServerSocket server;
    server.SetAddress("127.0.0.1", port);
    ASSERT_TRUE(server.Init());
    ASSERT_TRUE(server.Run());

    ClientSocket client;
    ASSERT_TRUE(client.Init());
    ASSERT_TRUE(client.Connect("127.0.0.1", port));
    ASSERT_TRUE(client.Run());

    EXPECT_FALSE(client.Connect("127.0.0.1", port));
    EXPECT_FALSE(client.GetLastError().empty());

    client.Close(true);
    server.Close(true);
}

// ---------------------------------------------------------------------------
// Integration fixture — running server shared across tests
// ---------------------------------------------------------------------------

class ServerClientTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_port = FindFreePort();
        m_server.reset(new ServerSocket());
        m_server->SetAddress("127.0.0.1", m_port);
        ASSERT_TRUE(m_server->Init());
        ASSERT_TRUE(m_server->Run());
    }

    void TearDown() override
    {
        m_server->Close(true);
    }

    // Creates an initialised, connected, running client.
    std::unique_ptr<ClientSocket> MakeClient()
    {
        auto client = std::unique_ptr<ClientSocket>(new ClientSocket());
        if (!client->Init())            return nullptr;
        if (!client->Connect("127.0.0.1", m_port)) return nullptr;
        if (!client->Run())             return nullptr;
        return client;
    }

    int                           m_port{0};
    std::unique_ptr<ServerSocket> m_server;
};

// ---------------------------------------------------------------------------
// Connect / disconnect callbacks
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, ClientConnect_OnConnectFires)
{
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<int>        connect_count{0};

    m_server->OnConnected([&](int32_t)
    {
        connect_count++;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return connect_count.load() == 1; }));
    EXPECT_EQ(connect_count.load(), 1);

    client->Close(true);
}

TEST_F(ServerClientTest, ClientDisconnect_OnDisconnectFires)
{
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<int>        disconnect_count{0};

    m_server->OnDisconnected([&](int32_t)
    {
        disconnect_count++;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);
    Yield();

    client->Close(true);

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return disconnect_count.load() == 1; }));
    EXPECT_EQ(disconnect_count.load(), 1);
}

// ---------------------------------------------------------------------------
// Data flow: client → server
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, ClientSendsData_ServerReceives)
{
    const std::string payload = "hello server";

    std::mutex              mtx;
    std::condition_variable cv;
    std::string             received;

    m_server->OnDataReady([&](int32_t, ByteArray&& data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        received.assign(data.begin(), data.end());
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);
    Yield();

    ASSERT_TRUE(client->Write(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return !received.empty(); }));
    EXPECT_EQ(received, payload);

    client->Close(true);
}

TEST_F(ServerClientTest, MultipleMessages_SameClient)
{
    const int               msg_count = 5;
    std::mutex              mtx;
    std::condition_variable cv;
    std::vector<std::string> received;

    m_server->OnDataReady([&](int32_t, ByteArray&& data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        received.emplace_back(data.begin(), data.end());
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);
    Yield();

    for (int i = 0; i < msg_count; i++)
    {
        std::string msg = "msg" + std::to_string(i);
        ASSERT_TRUE(client->Write(
            reinterpret_cast<const uint8_t*>(msg.data()), msg.size()));
        Yield(); // let the server process each one
    }

    EXPECT_TRUE(WaitFor(mtx, cv,
        [&]{ return static_cast<int>(received.size()) >= msg_count; }, 5000));
    EXPECT_EQ(static_cast<int>(received.size()), msg_count);

    client->Close(true);
}

// ---------------------------------------------------------------------------
// Data flow: server → client
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, ServerWritesBack_ClientReceives)
{
    const std::string server_msg = "hello client";

    std::mutex              mtx;
    std::condition_variable cv;
    std::string             received;
    std::atomic<int32_t>    client_idx{-1};

    m_server->OnConnected([&](int32_t idx)
    {
        client_idx = idx;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);

    client->SetOnData([&](ByteArray&& data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        received.assign(data.begin(), data.end());
        cv.notify_all();
    });

    // Wait for OnConnected to fire so we have the idx
    ASSERT_TRUE(WaitFor(mtx, cv, [&]{ return client_idx.load() != -1; }));

    ASSERT_TRUE(m_server->Write(
        client_idx.load(),
        reinterpret_cast<const uint8_t*>(server_msg.data()),
        server_msg.size()));

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return !received.empty(); }));
    EXPECT_EQ(received, server_msg);

    client->Close(true);
}

// ---------------------------------------------------------------------------
// Echo round-trip
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, EchoRoundTrip)
{
    const std::string payload = "ping";

    std::mutex              mtx;
    std::condition_variable cv;
    std::string             echoed;

    // Server echoes everything back
    m_server->OnDataReady([&](int32_t idx, ByteArray&& data)
    {
        m_server->Write(idx, data.data(), data.size());
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);

    client->SetOnData([&](ByteArray&& data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        echoed.assign(data.begin(), data.end());
        cv.notify_all();
    });

    Yield();

    ASSERT_TRUE(client->Write(
        reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return !echoed.empty(); }));
    EXPECT_EQ(echoed, payload);

    client->Close(true);
}

// ---------------------------------------------------------------------------
// Large data — spans multiple BUFFER_SIZE reads
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, LargeData_SpansMultipleReads)
{
    const size_t total_size = 8192; // 8× BUFFER_SIZE
    std::vector<uint8_t> payload(total_size);
    for (size_t i = 0; i < total_size; i++)
    {
        payload[i] = static_cast<uint8_t>(i & 0xFF);
    }

    std::mutex              mtx;
    std::condition_variable cv;
    std::vector<uint8_t>    received;

    m_server->OnDataReady([&](int32_t, ByteArray&& data)
    {
        std::lock_guard<std::mutex> lock(mtx);
        received.insert(received.end(), data.begin(), data.end());
        cv.notify_all();
    });

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);
    Yield();

    ASSERT_TRUE(client->Write(payload.data(), payload.size()));

    EXPECT_TRUE(WaitFor(mtx, cv,
        [&]{ return received.size() >= total_size; }, 5000));

    ASSERT_EQ(received.size(), total_size);
    EXPECT_EQ(received, payload);

    client->Close(true);
}

// ---------------------------------------------------------------------------
// Multiple clients
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, MultipleClients_AllConnect)
{
    const int               count = 5;
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<int>        connect_count{0};

    m_server->OnConnected([&](int32_t)
    {
        connect_count++;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    std::vector<std::unique_ptr<ClientSocket>> clients;
    for (int i = 0; i < count; i++)
    {
        auto c = MakeClient();
        ASSERT_NE(c, nullptr) << "client " << i << " failed to connect";
        clients.push_back(std::move(c));
    }

    EXPECT_TRUE(WaitFor(mtx, cv,
        [&]{ return connect_count.load() == count; }));
    EXPECT_EQ(connect_count.load(), count);

    for (auto& c : clients)
    {
        c->Close(true);
    }
}

TEST_F(ServerClientTest, MultipleClients_ConcurrentSend)
{
    const int    client_count = 4;
    const int    msg_per_client = 3;
    const int    expected = client_count * msg_per_client;

    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<int>        recv_count{0};

    m_server->OnDataReady([&](int32_t, ByteArray&&)
    {
        recv_count++;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    std::vector<std::unique_ptr<ClientSocket>> clients;
    for (int i = 0; i < client_count; i++)
    {
        auto c = MakeClient();
        ASSERT_NE(c, nullptr);
        clients.push_back(std::move(c));
    }
    Yield();

    // All clients send concurrently
    std::vector<std::thread> threads;
    for (int i = 0; i < client_count; i++)
    {
        threads.emplace_back([&, i]()
        {
            for (int j = 0; j < msg_per_client; j++)
            {
                std::string msg = "c" + std::to_string(i) + "m" + std::to_string(j);
                clients[i]->Write(
                    reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
                Yield();
            }
        });
    }
    for (auto& t : threads) { t.join(); }

    EXPECT_TRUE(WaitFor(mtx, cv,
        [&]{ return recv_count.load() >= expected; }, 5000));
    EXPECT_EQ(recv_count.load(), expected);

    for (auto& c : clients) { c->Close(true); }
}

// ---------------------------------------------------------------------------
// Close semantics
// ---------------------------------------------------------------------------

TEST_F(ServerClientTest, ServerClose_ClientFiresOnClose)
{
    std::mutex              mtx;
    std::condition_variable cv;
    std::atomic<bool>       on_close_fired{false};

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);

    client->SetOnClose([&]()
    {
        on_close_fired = true;
        std::lock_guard<std::mutex> lock(mtx);
        cv.notify_all();
    });

    Yield();

    // Server closes all connections
    m_server->Close(true);

    EXPECT_TRUE(WaitFor(mtx, cv, [&]{ return on_close_fired.load(); }));
    EXPECT_TRUE(on_close_fired.load());

    client->Close(true);
}

TEST_F(ServerClientTest, ExplicitClientClose_DoesNotFireOnClose)
{
    std::atomic<bool> on_close_fired{false};

    auto client = MakeClient();
    ASSERT_NE(client, nullptr);

    client->SetOnClose([&]() { on_close_fired = true; });

    Yield();

    // User-initiated close must NOT trigger the callback
    client->Close(true);

    Yield();
    EXPECT_FALSE(on_close_fired.load());
}

TEST_F(ServerClientTest, WriteAfterClose_ReturnsFalse)
{
    auto client = MakeClient();
    ASSERT_NE(client, nullptr);
    Yield();

    client->Close(true);

    uint8_t buf[] = {'x'};
    EXPECT_FALSE(client->Write(buf, sizeof(buf)));
}
