#ifndef REQUESTWEBSOCKET_H
#define REQUESTWEBSOCKET_H

#include "CommunicationClientBase.h"
#include "common.h"
#include "common_ws.h"

namespace WebSocketCpp
{

class RequestWebSocket
{
public:
    RequestWebSocket();
    bool             Parse(const ByteArray& data);
    bool             IsFinal() const;
    MessageType      GetType() const;
    void             SetType(MessageType type);
    size_t           GetSize() const;
    const ByteArray& GetData() const;
    void             SetData(const ByteArray& data);

    bool Send(CommunicationClientBase* communication) const;

private:
    constexpr static uint64_t MAX_WEBSOCKET_MESSAGE_SIZE = 10 * 1024 * 1024; // 10Mb

    ByteArray   m_data;
    bool        m_final       = false;
    size_t      m_size        = 0;
    MessageType m_messageType = MessageType::Undefined;
};

} // namespace WebSocketCpp

#endif // REQUESTWEBSOCKET_H
