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

#define WEB_SOCKET_CPP_VERSION        "0.2"
#define WEB_SOCKET_CPP_NAME           "WebCpp"
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
