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
    size_t                   test_count = 100;
    uint64_t                 delay_ms   = 50;

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    bool ServerHandler(const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data)
    {
        std::string str = StringUtil::ByteArray2String(data);
        arr_server.push_back(str);
        response.WriteText(data);

        return true;
    }

    bool ClientHandler(WebSocketCpp::ResponseWebSocket& response)
    {
        static size_t count = 0;
        std::string   str   = StringUtil::ByteArray2String(response.GetData());
        arr_client.push_back(str);
        count++;
        if (count >= test_count)
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
    if (server.Init())
    {
        server.OnMessage("/ws", [this](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
            return ServerHandler(request, response, data);
        });

        if (server.Run())
        {
            WebSocketCpp::WebSocketClient client;
            std::future<size_t>           fut;
            client.SetOnMessage([this](WebSocketCpp::ResponseWebSocket& response) -> bool {
                return ClientHandler(response);
            });
            if (client.Init())
            {
                if (client.Open("ws://127.0.0.1:8080/ws"))
                {
                    fut = std::async(std::launch::async, [&]() -> size_t {
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
                }
            }

            std::unique_lock<std::mutex> lock(mtx);
            bool                         success = cv.wait_for(lock, std::chrono::milliseconds(delay_ms * test_count * 5), [this]() { return finished.load(); });

            EXPECT_TRUE(success);
            server.Close();

            size_t hash            = fut.get();
            size_t calculated_hash = std::accumulate(
                arr_server.begin(), arr_server.end(), size_t(0),
                [](size_t hash, const std::string& s) { return hash |= _(s.c_str()); });

            EXPECT_EQ(hash, calculated_hash);
            EXPECT_EQ(arr_server, arr_client);
            EXPECT_EQ(arr_server.size(), test_count);
        }
    }
}

// Open server, then connect and disconnect client in a loop
TEST_F(WebSocketFixture, OneServerLoopClient)
{
    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetWsProtocol(Protocol::WS);
    config.SetWsServerPort(8080);

    WebSocketCpp::WebSocketServer server;
    ASSERT_TRUE(server.Init());
    server.OnMessage("/ws", [this](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
        return ServerHandler(request, response, data);
    });

    ASSERT_TRUE(server.Run());
    size_t hash = 0;
    for (size_t i = 0; i < test_count; i++)
    {
        WebSocketCpp::WebSocketClient client;
        std::future<size_t>           fut;
        ASSERT_TRUE(client.Init());

        ASSERT_TRUE(client.Open("ws://127.0.0.1:8080/ws"));
        std::string s = random_string();
        hash |= _(s.c_str());
        client.SendText(s);
        arr_client.push_back(s);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    server.Close();

    size_t calculated_hash = std::accumulate(
        arr_server.begin(), arr_server.end(), size_t(0),
        [](size_t hash, const std::string& s) { return hash |= _(s.c_str()); });

    EXPECT_EQ(arr_server, arr_client);
    EXPECT_EQ(hash, calculated_hash);
}
