/*
 * Copyright (c) 2026 ruslan@muhlinin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <gtest/gtest.h>

#include <condition_variable>
#include <csignal>
#include <future>
#include <mutex>
#include <random>
#include <thread>

#include "DebugPrint.h"
#include "Request.h"
#include "ResponseWebSocket.h"
#include "StringUtil.h"
#include "WebSocketClient.h"
#include "WebSocketServer.h"
#include "common.h"

using namespace WebSocketCpp;

class WebSocketFixture : public ::testing::Test
{
protected:
    std::vector<std::string> arr_server;
    std::vector<std::string> arr_client;
    std::mutex               mtx;
    std::condition_variable  cv;
    std::atomic<bool>        finished{false};
    size_t                   test_count   = 10;
    uint64_t                 delay_ms     = 50;
    size_t                   client_count = 10;
    size_t                   recv_count   = 0;

    void SetUp() override
    {
        //WebSocketCpp::DebugPrint::AllowPrint = false;
    }

    void TearDown() override
    {
    }

    bool ServerHandler(const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data)
    {
        std::string str = StringUtil::ByteArray2String(data);
        DebugPrint() << "server received " << str << " from #" << request.GetConnectionID() << std::endl;
        arr_server.push_back(str);
        response.WriteText(data);

        return true;
    }

    bool ClientHandler(WebSocketCpp::ResponseWebSocket& response)
    {
        std::string str = StringUtil::ByteArray2String(response.GetData());
        DebugPrint() << "client received " << str << std::endl;
        arr_client.push_back(str);

        if (++recv_count >= test_count)
        {
            finished = true;
            cv.notify_one();
        }
        return true;
    }

    std::string random_string(size_t min_length = 5, size_t max_length = 20)
    {
        constexpr static char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        thread_local std::mt19937             rng{std::random_device{}()};
        std::uniform_int_distribution<size_t> length_dist(min_length, max_length);
        std::uniform_int_distribution<size_t> char_dist(0, sizeof(chars) - 2);

        size_t      length = length_dist(rng);
        std::string result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i)
            result += chars[char_dist(rng)];

        return result;
    }
};

// Open server and client simultaneously, exchange random data as ping-pong,
// compute a hash of all sent data and verify it matches the received data.
TEST_F(WebSocketFixture, OneServerOneClient)
{
    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetWsProtocol(Protocol::WS);
    config.SetWsServerPort(8080);

    WebSocketCpp::WebSocketServer server;
    if (!server.Init())
    {
        FAIL() << "server.Init() failed: " << server.GetLastError();
    }

    server.OnMessage("/ws", [this](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
        return ServerHandler(request, response, data);
    });

    if (!server.Run())
    {
        FAIL() << "server.Run() failed: " << server.GetLastError();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    WebSocketCpp::WebSocketClient client;
    client.SetOnMessage([this](WebSocketCpp::ResponseWebSocket& response) -> bool {
        return ClientHandler(response);
    });

    if (!client.Init())
    {
        server.Close();
        FAIL() << "client.Init() failed: " << client.GetLastError();
    }

    if (!client.Open("ws://127.0.0.1:8080/ws"))
    {
        server.Close();
        FAIL() << "client.Open() failed: " << client.GetLastError();
    }

    std::future<size_t> fut = std::async(std::launch::async, [&]() -> size_t {
        size_t hash = 0;
        for (size_t i = 0; i < test_count; i++)
        {
            std::string s = random_string();
            hash |= _(s.c_str());
            client.SendText(s);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        return hash;
    });

    {
        std::unique_lock<std::mutex> lock(mtx);
        bool success = cv.wait_for(
            lock,
            std::chrono::milliseconds(delay_ms * test_count * 5),
            [this]() { return finished.load(); });

        EXPECT_TRUE(success) << "timed out waiting for all tests";
    }

    server.Close();

    size_t hash = fut.get();

    size_t calculated_hash = std::accumulate(
        arr_server.begin(), arr_server.end(), size_t(0),
        [](size_t h, const std::string& s) { return h |= _(s.c_str()); });

    EXPECT_EQ(hash, calculated_hash);
    EXPECT_EQ(arr_server, arr_client);
    EXPECT_EQ(arr_server.size(), test_count);
}


// Open server, then connect and disconnect clients in a loop
TEST_F(WebSocketFixture, OneServerLoopClient)
{
    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetWsProtocol(Protocol::WS);
    config.SetWsServerPort(8080);
    size_t                  count{};
    std::mutex              client_mtx;
    std::condition_variable client_cv;

    WebSocketCpp::WebSocketServer server;
    ASSERT_TRUE(server.Init());
    server.OnMessage("/ws", [this, &count, &client_cv](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
        count++;
        bool ret = ServerHandler(request, response, data);
        client_cv.notify_one();
        return ret;
    });

    ASSERT_TRUE(server.Run());
    size_t hash = 0;
    for (size_t i = 0; i < test_count; i++)
    {
        WebSocketCpp::WebSocketClient client;
        std::future<size_t>           fut;
        ASSERT_TRUE(client.Init());

        client.Open("ws://127.0.0.1:8080/ws");
        std::string s = random_string();
        hash |= _(s.c_str());
        client.SendText(s);
        arr_client.push_back(s);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::unique_lock<std::mutex> lock(client_mtx);
    bool                         success = client_cv.wait_for(lock, std::chrono::milliseconds(50), [&count, this]() { return count == test_count; });
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.Close();

    EXPECT_TRUE(success);

    size_t calculated_hash = std::accumulate(
        arr_server.begin(), arr_server.end(), size_t(0),
        [](size_t hash, const std::string& s) { return hash |= _(s.c_str()); });

    EXPECT_EQ(arr_server, arr_client);
    EXPECT_EQ(hash, calculated_hash);
}

// Open server, then connect N clients in a loop
TEST_F(WebSocketFixture, OneServerNClient)
{
    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetWsProtocol(Protocol::WS);
    config.SetWsServerPort(8080);
    config.SetMaxClientCount(client_count);

    WebSocketCpp::WebSocketServer server;
    ASSERT_TRUE(server.Init());

    server.OnMessage("/ws", [this](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
        return ServerHandler(request, response, data);
    });

    ASSERT_TRUE(server.Run());

    std::vector<std::thread> clients;
    std::vector<size_t>      results;
    clients.resize(client_count);
    results.resize(client_count);

    for (size_t i = 0; i < client_count; i++)
    {
        clients[i] = std::thread([this, i](size_t& result) {
            std::mutex                    client_mtx;
            std::condition_variable       client_cv;
            WebSocketCpp::WebSocketClient client;

            std::string received;
            client.SetOnMessage([&received, &client_cv, i](WebSocketCpp::ResponseWebSocket& response) -> bool {
                received = StringUtil::ByteArray2String(response.GetData());
                DebugPrint() << "client #" << i << " received: " << received << std::endl;
                client_cv.notify_one();
                return true;
            });

            if (client.Open("ws://127.0.0.1:8080/ws"))
            {
                for (size_t j = 0; j < test_count; j++)
                {
                    std::string sent = random_string();
                    client.SendText(sent);
                    std::unique_lock<std::mutex> lock(client_mtx);
                    bool                         success = client_cv.wait_for(lock, std::chrono::milliseconds(500), [&received]() { return !received.empty(); });
                    if (success && sent == received)
                    {
                        result++;
                    }
                    received.clear();
                }
            }

            client.Close();
        },
            std::ref(results[i]));
    }

    for (size_t i = 0; i < client_count; i++)
    {
        clients[i].join();
        EXPECT_EQ(test_count, results[i]) << " client #" << i;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.Close();
}

#ifdef WITH_OPENSSL
// Same as OneServerNClient but over WSS (TLS)
TEST_F(WebSocketFixture, OneServerNClientSsl)
{
    const std::string cert = TEST_CERT_DIR "/test_cert.pem";
    const std::string key  = TEST_CERT_DIR "/test_key.pem";

    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetWsProtocol(Protocol::WSS);
    config.SetWsServerPort(8443);
    config.SetMaxClientCount(client_count);
    config.SetSslSertificate(cert);
    config.SetSslKey(key);

    WebSocketCpp::WebSocketServer server;
    ASSERT_TRUE(server.Init()) << server.GetLastError();

    server.OnMessage("/ws", [this](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
        return ServerHandler(request, response, data);
    });

    ASSERT_TRUE(server.Run()) << server.GetLastError();

    std::vector<std::thread> clients;
    std::vector<size_t>      results;
    clients.resize(client_count);
    results.resize(client_count);

    for (size_t i = 0; i < client_count; i++)
    {
        clients[i] = std::thread([this, i, &cert, &key](size_t& result) {
            std::mutex                    client_mtx;
            std::condition_variable       client_cv;
            WebSocketCpp::WebSocketClient client;

            std::string received;
            client.SetOnMessage([&received, &client_cv, i](WebSocketCpp::ResponseWebSocket& response) -> bool {
                received = StringUtil::ByteArray2String(response.GetData());
                DebugPrint() << "client #" << i << " received: " << received << std::endl;
                client_cv.notify_one();
                return true;
            });

            if (client.Open("wss://127.0.0.1:8443/ws"))
            {
                for (size_t j = 0; j < test_count; j++)
                {
                    std::string sent = random_string();
                    client.SendText(sent);
                    std::unique_lock<std::mutex> lock(client_mtx);
                    bool success = client_cv.wait_for(lock, std::chrono::milliseconds(500), [&received]() { return !received.empty(); });
                    if (success && sent == received)
                    {
                        result++;
                    }
                    received.clear();
                }
            }
            else
            {
                DebugPrint() << "client #" << i << " Open failed: " << client.GetLastError() << std::endl;
            }

            client.Close();
        },
            std::ref(results[i]));
    }

    for (size_t i = 0; i < client_count; i++)
    {
        clients[i].join();
        EXPECT_EQ(test_count, results[i]) << " client #" << i;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.Close();

    // Reset protocol back to WS so other tests are not affected
    config.SetWsProtocol(Protocol::WS);
}
#endif // WITH_OPENSSL
