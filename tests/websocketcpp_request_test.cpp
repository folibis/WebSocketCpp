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
#include <random>
#include "Request.h"

using namespace WebSocketCpp;

// ─────────────────────────────────────────────────────────────
// Random generators
// ─────────────────────────────────────────────────────────────
class RandomGen
{
    std::mt19937 rng;

public:
    RandomGen() : rng(std::random_device{}()) {}

    std::string AlphaNumeric(size_t min_len = 5, size_t max_len = 15)
    {
        const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::uniform_int_distribution<size_t> len_dist(min_len, max_len);
        std::uniform_int_distribution<size_t> char_dist(0, sizeof(chars) - 2);

        size_t len = len_dist(rng);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += chars[char_dist(rng)];
        return result;
    }

    std::string Path()
    {
        std::uniform_int_distribution<size_t> depth_dist(1, 4);
        size_t depth = depth_dist(rng);
        std::string path = "/";
        for (size_t i = 0; i < depth; ++i)
        {
            path += AlphaNumeric(3, 10);
            if (i < depth - 1) path += "/";
        }
        return path;
    }

    std::string Host()
    {
        return AlphaNumeric(5, 10) + "." + AlphaNumeric(3, 8) + ".com";
    }

    int Port()
    {
        std::uniform_int_distribution<int> dist(1024, 65535);
        return dist(rng);
    }

    std::string Base64(size_t len = 16)
    {
        const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::uniform_int_distribution<size_t> dist(0, sizeof(chars) - 2);
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i)
            result += chars[dist(rng)];
        return result + "==";
    }
};

// ─────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────
class RequestTest : public ::testing::Test
{
protected:
    Request request;
    RandomGen rand;

    // Helper to build a basic HTTP request
    ByteArray BuildBasicRequest(const std::string& method,
        const std::string& path,
        const std::string& host,
        const std::string& version = "HTTP/1.1")
    {
        std::string req = method + " " + path + " " + version + "\r\n";
        req += "Host: " + host + "\r\n";
        req += "\r\n";
        return ByteArray(req.begin(), req.end());
    }

    // Helper to build WebSocket upgrade request
    ByteArray BuildWebSocketUpgradeRequest(const std::string& path,
        const std::string& host,
        const std::string& wsKey,
        const std::string& version = "13")
    {
        std::string req = "GET " + path + " HTTP/1.1\r\n";
        req += "Host: " + host + "\r\n";
        req += "Upgrade: websocket\r\n";
        req += "Connection: Upgrade\r\n";
        req += "Sec-WebSocket-Key: " + wsKey + "\r\n";
        req += "Sec-WebSocket-Version: " + version + "\r\n";
        req += "\r\n";
        return ByteArray(req.begin(), req.end());
    }
};


// ─────────────────────────────────────────────────────────────
// Construction & Default State
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, DefaultConstructor_InitializesCorrectly)
{
    EXPECT_EQ(request.GetConnectionID(), -1);
    EXPECT_EQ(request.GetMethod(), Method::Undefined);
    EXPECT_EQ(request.GetHttpVersion(), "HTTP/1.1");
    EXPECT_TRUE(request.GetRemote().empty());
    EXPECT_EQ(request.GetRequestLineLength(), 0);
    EXPECT_EQ(request.GetSession(), nullptr);
}


// ─────────────────────────────────────────────────────────────
// Parse Request Line - Basic HTTP
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Parse_SimpleGetRequest)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    ByteArray data = BuildBasicRequest("GET", path, host);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetMethod(), Method::GET);
    EXPECT_NE(request.GetUrl().GetPath().find(path.substr(1)), std::string::npos); // path starts with /
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
    EXPECT_EQ(request.GetHttpVersion(), "HTTP/1.1");
}

TEST_F(RequestTest, Parse_PostRequest)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    ByteArray data = BuildBasicRequest("POST", path, host);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetMethod(), Method::POST);
    EXPECT_NE(request.GetUrl().GetPath().find(path.substr(1)), std::string::npos);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}

TEST_F(RequestTest, Parse_RequestWithQuery)
{
    std::string path = rand.Path();
    std::string key1 = rand.AlphaNumeric(3, 8);
    std::string val1 = rand.AlphaNumeric(5, 10);
    std::string key2 = rand.AlphaNumeric(3, 8);
    std::string val2 = rand.AlphaNumeric(5, 10);
    std::string fullPath = path + "?" + key1 + "=" + val1 + "&" + key2 + "=" + val2;
    std::string host = rand.Host();

    ByteArray data = BuildBasicRequest("GET", fullPath, host);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetUrl().GetQueryValue(key1), val1);
    EXPECT_EQ(request.GetUrl().GetQueryValue(key2), val2);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}

TEST_F(RequestTest, Parse_UnsupportedMethod_ReturnsFalse)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    ByteArray data = BuildBasicRequest("REQUEST", path, host);
    EXPECT_FALSE(request.Parse(data));
}

TEST_F(RequestTest, Parse_MalformedRequestLine_ReturnsFalse)
{
    std::string req = "GET\r\n\r\n";
    ByteArray data(req.begin(), req.end());
    EXPECT_FALSE(request.Parse(data));
}


// ─────────────────────────────────────────────────────────────
// WebSocket Protocol Detection
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, GetProtocol_WebSocketUpgrade_ReturnsWS)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::WS);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
    EXPECT_EQ(request.GetHeader().GetHeader("Sec-WebSocket-Key"), wsKey);
}

TEST_F(RequestTest, GetProtocol_RegularHTTP_ReturnsHTTP)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    ByteArray data = BuildBasicRequest("GET", path, host);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::HTTP);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}

TEST_F(RequestTest, GetProtocol_UpgradeCaseInsensitive)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: WebSocket\r\n";  // Mixed case
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::WS);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}

TEST_F(RequestTest, GetProtocol_UpgradeToOther_ReturnsHTTP)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: h2c\r\n";  // HTTP/2 upgrade, not websocket
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::HTTP);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}


// ─────────────────────────────────────────────────────────────
// WebSocket Headers Parsing
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Parse_WebSocketHeaders_ParsedCorrectly)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);

    EXPECT_TRUE(request.Parse(data));

    const HttpHeader& header = request.GetHeader();
    EXPECT_EQ(header.GetHeader(Header::HeaderType::Host), host);
    EXPECT_EQ(header.GetHeader(Header::HeaderType::Upgrade), "websocket");
    EXPECT_EQ(header.GetHeader(Header::HeaderType::Connection), "Upgrade");
    EXPECT_EQ(header.GetHeader("Sec-WebSocket-Key"), wsKey);
    EXPECT_EQ(header.GetHeader("Sec-WebSocket-Version"), "13");
}

TEST_F(RequestTest, Parse_WebSocketPath_ParsedCorrectly)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_NE(request.GetUrl().GetPath().find(path.substr(1)), std::string::npos); // path starts with /
}

TEST_F(RequestTest, Parse_WebSocketWithQueryParams)
{
    std::string path = rand.Path();
    std::string token = rand.AlphaNumeric(10, 20);
    std::string room = rand.AlphaNumeric(5, 10);
    std::string fullPath = path + "?token=" + token + "&room=" + room;
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);

    ByteArray data = BuildWebSocketUpgradeRequest(fullPath, host, wsKey);

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetUrl().GetQueryValue("token"), token);
    EXPECT_EQ(request.GetUrl().GetQueryValue("room"), room);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::Host), host);
}


// ─────────────────────────────────────────────────────────────
// Request Size Calculation
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, GetRequestSize_NoBody_CorrectSize)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);
    EXPECT_TRUE(request.Parse(data));

    size_t expectedSize = request.GetRequestLineLength() + 2 +  // request line + CRLF
                          request.GetHeader().GetHeaderSize() + 4; // headers + CRLFCRLF
    EXPECT_EQ(request.GetRequestSize(), expectedSize);
}

TEST_F(RequestTest, GetRequestSize_WithBody_IncludesBodySize)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string body = rand.AlphaNumeric(20, 50);

    std::string req = "POST " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    req += "\r\n";
    req += body;
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetHeader().GetBodySize(), body.size());
    EXPECT_GT(request.GetRequestSize(), request.GetRequestLineLength());
}


// ─────────────────────────────────────────────────────────────
// Header Manipulation
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, SetHeader_ModifiesHeader)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string userAgent = rand.AlphaNumeric(10, 20);
    ByteArray data = BuildBasicRequest("GET", path, host);
    EXPECT_TRUE(request.Parse(data));

    request.GetHeader().SetHeader(Header::HeaderType::UserAgent, userAgent);
    EXPECT_EQ(request.GetHeader().GetHeader(Header::HeaderType::UserAgent), userAgent);
}

TEST_F(RequestTest, GetHeader_NonExistent_ReturnsEmpty)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    ByteArray data = BuildBasicRequest("GET", path, host);
    EXPECT_TRUE(request.Parse(data));

    EXPECT_TRUE(request.GetHeader().GetHeader("X-Custom-Header").empty());
}


// ─────────────────────────────────────────────────────────────
// Arguments (route parameters)
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, SetArg_GetArg_WorksCorrectly)
{
    std::string key1 = rand.AlphaNumeric(5, 10);
    std::string val1 = rand.AlphaNumeric(10, 20);
    std::string key2 = rand.AlphaNumeric(5, 10);
    std::string val2 = rand.AlphaNumeric(10, 20);

    request.SetArg(key1, val1);
    request.SetArg(key2, val2);

    EXPECT_EQ(request.GetArg(key1), val1);
    EXPECT_EQ(request.GetArg(key2), val2);
}

TEST_F(RequestTest, GetArg_NonExistent_ReturnsEmpty)
{
    EXPECT_TRUE(request.GetArg("nonexistent").empty());
}


// ─────────────────────────────────────────────────────────────
// Remote Address
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, SetRemote_GetRemote)
{
    std::string remote = rand.Host() + ":" + std::to_string(rand.Port());
    request.SetRemote(remote);
    EXPECT_EQ(request.GetRemote(), remote);
}


// ─────────────────────────────────────────────────────────────
// Connection ID
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, SetConnectionID_GetConnectionID)
{
    int connId = rand.Port();  // reuse port generator for random int
    request.SetConnectionID(connId);
    EXPECT_EQ(request.GetConnectionID(), connId);
}


// ─────────────────────────────────────────────────────────────
// Session Management
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, SetSession_GetSession)
{
    Session* mockSession = reinterpret_cast<Session*>(0x1234);  // mock pointer
    request.SetSession(mockSession);
    EXPECT_EQ(request.GetSession(), mockSession);
}


// ─────────────────────────────────────────────────────────────
// Clear
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Clear_ResetsAllFields)
{
    // Setup a fully populated request
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    std::string argKey = rand.AlphaNumeric(5, 10);
    std::string argVal = rand.AlphaNumeric(10, 20);
    int connId = rand.Port();
    std::string remote = rand.Host() + ":" + std::to_string(rand.Port());

    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);
    request.Parse(data);
    request.SetConnectionID(connId);
    request.SetRemote(remote);
    request.SetArg(argKey, argVal);
    request.SetSession(reinterpret_cast<Session*>(0x1234));

    // Clear it
    request.Clear();

    // Verify everything is reset
    EXPECT_EQ(request.GetConnectionID(), -1);
    EXPECT_EQ(request.GetMethod(), Method::Undefined);
    EXPECT_EQ(request.GetHttpVersion(), "HTTP/1.1");
    EXPECT_EQ(request.GetRequestLineLength(), 0);
    EXPECT_TRUE(request.GetRemote().empty());
    EXPECT_EQ(request.GetSession(), nullptr);
    EXPECT_TRUE(request.GetArg(argKey).empty());
}

TEST_F(RequestTest, Clear_ThenReParse_Works)
{
    std::string path1 = rand.Path();
    std::string host1 = rand.Host();
    ByteArray data1 = BuildBasicRequest("GET", path1, host1);
    request.Parse(data1);
    EXPECT_FALSE(request.GetUrl().GetPath().empty());

    request.Clear();

    std::string path2 = rand.Path();
    std::string host2 = rand.Host();
    ByteArray data2 = BuildBasicRequest("POST", path2, host2);
    request.Parse(data2);
    EXPECT_EQ(request.GetMethod(), Method::POST);
    EXPECT_FALSE(request.GetUrl().GetPath().empty());
}


// ─────────────────────────────────────────────────────────────
// ToString
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, ToString_ContainsBasicInfo)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    int connId = rand.Port();

    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);
    request.Parse(data);
    request.SetConnectionID(connId);

    std::string str = request.ToString();
    EXPECT_NE(str.find("connID"), std::string::npos);
    EXPECT_NE(str.find(std::to_string(connId)), std::string::npos);
    EXPECT_NE(str.find("headers"), std::string::npos);
}


// ─────────────────────────────────────────────────────────────
// Edge Cases
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Parse_EmptyData_ReturnsFalse)
{
    ByteArray empty;
    EXPECT_FALSE(request.Parse(empty));
}

TEST_F(RequestTest, Parse_IncompleteRequest_ReturnsFalse)
{
    std::string path = rand.Path();
    std::string req = "GET " + path + " HTTP/1.1\r\n";  // no CRLFCRLF
    ByteArray data(req.begin(), req.end());
    EXPECT_FALSE(request.Parse(data));
}

TEST_F(RequestTest, Parse_MultipleCalls_Idempotent)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string wsKey = rand.Base64(22);
    ByteArray data = BuildWebSocketUpgradeRequest(path, host, wsKey);
    EXPECT_TRUE(request.Parse(data));

    // Parse again should still work (stateful incremental parsing)
    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::WS);
}


// ─────────────────────────────────────────────────────────────
// WebSocket-Specific Edge Cases
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Parse_WebSocketWithoutSecKey_StillParsable)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Connection: Upgrade\r\n";
    // Missing Sec-WebSocket-Key
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::WS);
    EXPECT_TRUE(request.GetHeader().GetHeader("Sec-WebSocket-Key").empty());
}

TEST_F(RequestTest, Parse_WebSocketDifferentVersion)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string version = "8";  // older version
    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Sec-WebSocket-Version: " + version + "\r\n";
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetHeader().GetHeader("Sec-WebSocket-Version"), version);
}

TEST_F(RequestTest, Parse_WebSocketWithSubprotocol)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string proto1 = rand.AlphaNumeric(4, 8);
    std::string proto2 = rand.AlphaNumeric(4, 8);
    std::string protocols = proto1 + ", " + proto2;

    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Sec-WebSocket-Protocol: " + protocols + "\r\n";
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetHeader().GetHeader("Sec-WebSocket-Protocol"), protocols);
}

TEST_F(RequestTest, Parse_WebSocketWithExtensions)
{
    std::string path = rand.Path();
    std::string host = rand.Host();
    std::string ext = rand.AlphaNumeric(10, 20);

    std::string req = "GET " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Sec-WebSocket-Extensions: " + ext + "\r\n";
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetHeader().GetHeader("Sec-WebSocket-Extensions"), ext);
}


// ─────────────────────────────────────────────────────────────
// Real-World WebSocket Request
// ─────────────────────────────────────────────────────────────
TEST_F(RequestTest, Parse_RealWorldWebSocketRequest)
{
    std::string path = rand.Path();
    std::string user = rand.AlphaNumeric(5, 10);
    std::string room = rand.AlphaNumeric(5, 10);
    std::string host = rand.Host();
    int port = rand.Port();
    std::string wsKey = rand.Base64(22);
    std::string protocol = rand.AlphaNumeric(4, 8);
    std::string origin = "http://" + rand.Host();
    std::string userAgent = "Mozilla/5.0";

    std::string req = "GET " + path + "?user=" + user + "&room=" + room + " HTTP/1.1\r\n";
    req += "Host: " + host + ":" + std::to_string(port) + "\r\n";
    req += "Upgrade: websocket\r\n";
    req += "Connection: Upgrade\r\n";
    req += "Sec-WebSocket-Key: " + wsKey + "\r\n";
    req += "Sec-WebSocket-Protocol: " + protocol + "\r\n";
    req += "Sec-WebSocket-Version: 13\r\n";
    req += "Origin: " + origin + "\r\n";
    req += "User-Agent: " + userAgent + "\r\n";
    req += "\r\n";
    ByteArray data(req.begin(), req.end());

    EXPECT_TRUE(request.Parse(data));
    EXPECT_EQ(request.GetProtocol(), Protocol::WS);
    EXPECT_EQ(request.GetMethod(), Method::GET);
    EXPECT_FALSE(request.GetUrl().GetPath().empty());
    EXPECT_EQ(request.GetUrl().GetQueryValue("user"), user);
    EXPECT_EQ(request.GetUrl().GetQueryValue("room"), room);
    EXPECT_EQ(request.GetHeader().GetHeader("Sec-WebSocket-Key"), wsKey);
}

