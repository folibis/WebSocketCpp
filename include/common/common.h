/*
 *  * Copyright (c) 2026 ruslan@muhlinin.com
 *  * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *  * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *  */

#ifndef WEB_SOCKET_CPP_COMMON_H
#define WEB_SOCKET_CPP_COMMON_H

#include <cstdint>
#include <string>
#include <vector>

namespace WebSocketCpp
{

enum class Method
{
    Undefined = 0,
    OPTIONS,
    GET,
    HEAD,
    POST,
    PUT,
    DELETE,
    TRACE,
    CONNECT,
    WEBSOCKET,
};

enum class Protocol
{
    Undefined = 0,
    HTTP,
    HTTPS,
    WS,
    WSS,
};

std::string Protocol2String(Protocol protocol);
Protocol    String2Protocol(const std::string& str);
Method      String2Method(const std::string& str);
std::string Method2String(Method method);

#define CR       '\r'
#define LF       '\n'
#define CRLF     '\r', '\n'
#define CRLFCRLF '\r', '\n', '\r', '\n'

#define WEB_SOCKET_CPP_VERSION        "0.3"
#define WEB_SOCKET_CPP_NAME           "WebSocketCpp"
#define WEB_SOCKET_CPP_CANONICAL_NAME WEB_SOCKET_CPP_NAME " " WEB_SOCKET_CPP_VERSION

using ByteArray = std::vector<uint8_t>;

struct point
{
    size_t p1;
    size_t p2;
};
using PointArray = std::vector<point>;

constexpr unsigned long long operator"" _Kb(unsigned long long num)
{
    return num * 1024LL;
}
constexpr unsigned long long operator"" _Mb(unsigned long long num)
{
    return num * 1024LL * 1024LL;
}
constexpr unsigned long long operator"" _Gb(unsigned long long num)
{
    return num * 1024LL * 1024LL * 1024LL;
}

constexpr uint64_t mix(char m, uint64_t s)
{
    return ((s << 7) + ~(s >> 3)) + static_cast<uint64_t>(~m);
}

constexpr uint64_t _(const char* str)
{
    return (*str) ? mix(*str, _(str + 1)) : 0;
}

constexpr uint64_t operator"" _(const char* str)
{
    return _(str);
}

} // namespace WebSocketCpp

#endif // WEB_SOCKET_CPP_COMMON_H
