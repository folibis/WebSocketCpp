#include "RouteWebSocket.h"

using namespace WebSocketCpp;

RouteWebSocket::RouteWebSocket(const std::string& path)
    : Route(path, Method::WEBSOCKET)
{
}

RouteWebSocket::RouteWebSocket(const std::string& path, RouteFuncMessage message_func, RouteFuncRequest request_func)
    : Route(path, Method::WEBSOCKET),
      m_funcMessage(std::move(message_func)),
      m_funcRequest(std::move(request_func))

{
}

bool RouteWebSocket::SetFunctionRequest(RouteFuncRequest f)
{
    m_funcRequest = std::move(f);
    return true;
}

const RouteWebSocket::RouteFuncRequest& RouteWebSocket::GetFunctionRequest() const
{
    return m_funcRequest;
}

bool RouteWebSocket::SetFunctionMessage(RouteFuncMessage f)
{
    m_funcMessage = std::move(f);
    return true;
}

const RouteWebSocket::RouteFuncMessage& RouteWebSocket::GetFunctionMessage() const
{
    return m_funcMessage;
}
