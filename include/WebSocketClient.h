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
 *
 */

#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <functional>
#include <memory>
#include <string>

#include "CommunicationClientBase.h"
#include "Config.h"
#include "IErrorable.h"
#include "IRunnable.h"
#include "Request.h"
#include "ResponseWebSocket.h"

namespace WebSocketCpp
{

class WebSocketClient : public IErrorable, public IRunnable
{
public:
    enum class State
    {
        Undefined = 0,
        Initialized,
        Connected,
        HandshakeSent,
        Handshake,
        HandshakeFailed,
        BinaryMessage,
        Closed,
    };

    WebSocketClient();
    ~WebSocketClient();
    bool Init() override;
    bool Init(const WebSocketCpp::Config& config);
    bool Run() override;
    bool Close(bool wait = true) override;
    bool WaitFor() override;
    bool Open(Request& request);
    bool Open(const std::string& address);
    bool SendText(const ByteArray& data);
    bool SendText(const std::string& data);
    bool SendBinary(const ByteArray& data);
    bool SendBinary(const std::string& data);
    bool SendPing();

    void SetOnConnect(const std::function<void(bool)>& callback);
    void SetOnClose(const std::function<void(void)>& callback);
    void SetOnError(const std::function<void(const std::string&)>& callback);
    void SetOnMessage(const std::function<void(ResponseWebSocket&)>& callback);
    void SetProgressCallback(const std::function<void(size_t, size_t)>& callback);
    void SetOnStateChanged(const std::function<void(State)>& callback);

protected:
    void OnDataReady(ByteArray&& data);
    void OnClosed();
    bool InitConnection(const Url& url);
    void SetState(State state);

private:
    std::shared_ptr<CommunicationClientBase> m_connection = nullptr;
    Config&                                  m_config;
    std::function<void(bool)>                m_connectCallback  = nullptr;
    std::function<void(void)>                m_closeCallback    = nullptr;
    std::function<void(const std::string&)>  m_errorCallback    = nullptr;
    std::function<void(ResponseWebSocket&)>  m_messageCallback  = nullptr;
    std::function<void(size_t, size_t)>      m_progressCallback = nullptr;
    std::function<void(State)>               m_stateCallback    = nullptr;
    State                                    m_state            = State::Undefined;
    std::string                              m_key;
    ByteArray                                m_data;
    Mutex                                    m_connect_mtx;
    Signal                                   m_connect_cv;
    Mutex                                    m_read_mtx;
};

} // namespace WebSocketCpp

#endif // WEBSOCKETCLIENT_H
