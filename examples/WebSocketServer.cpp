/*
 *
 * Copyright (c) 2021 ruslan@muhlinin.com
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
 *
 */

/*
 * WebSocketServer - a simple WebSocket server that just sends
 * periodically a random number in specified range and delay
 */

#include "WebSocketServer.h"

#include <csignal>

#include "DebugPrint.h"
#include "Platform.h"
#include "Request.h"
#include "ResponseWebSocket.h"
#include "StringUtil.h"
#include "ThreadWorker.h"
#include "common.h"
#include "example_common.h"

using namespace WebSocketCpp;
static WebSocketCpp::WebSocketServer* wsServerPtr = nullptr;

void handle_sigint(int)
{
    wsServerPtr->Close(false);
}

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    int              port_ws       = DEFAULT_WS_PORT;
    WebSocketCpp::Protocol http_protocol = DEFAULT_HTTP_PROTOCOL;
    WebSocketCpp::Protocol ws_protocol   = DEFAULT_WS_PROTOCOL;

    auto cmdline = CommandLine::Parse(argc, argv);

    if (cmdline.Exists("-h"))
    {
        cmdline.PrintUsage(true, true);
        exit(0);
    }

    if (cmdline.Exists("-v"))
    {
        WebSocketCpp::DebugPrint::AllowPrint = true;
    }

    int v;
    if (StringUtil::String2int(cmdline.Get("-p"), v))
    {
        port_ws = v;
    }

    std::string s;
    if (cmdline.Set("-r", s) == true)
    {
        ws_protocol = WebSocketCpp::String2Protocol(s);
    }

    int                     connID = (-1);
    int                     min = 1, max = 100;
    WebSocketCpp::WebSocketServer wsServer;
    wsServerPtr = &wsServer;
    WebSocketCpp::ThreadWorker task;
    task.SetFunction([&](bool& running) -> void* {
        WebSocketCpp::ResponseWebSocket response(connID);
        StringUtil::RandInit();
        while (running)
        {
            int               num  = StringUtil::GetRand(min, max);
            WebSocketCpp::ByteArray data = StringUtil::String2ByteArray(std::to_string(num));
            response.WriteText(data);
            wsServer.SendResponse(response);
            WebSocketCpp::Sleep(1);
        }

        return nullptr;
    });

    WebSocketCpp::Config& config = WebSocketCpp::Config::Instance();
    config.SetRoot(PUB);
    config.SetWsProtocol(ws_protocol);
    config.SetWsServerPort(port_ws);
    config.SetSslSertificate(SSL_CERT);
    config.SetSslKey(SSL_KEY);

    if (wsServer.Init())
    {
        wsServer.OnMessage("/ws", [&](const WebSocketCpp::Request& request, WebSocketCpp::ResponseWebSocket& response, const WebSocketCpp::ByteArray& data) -> bool {
            std::string d = StringUtil::ByteArray2String(data);
            switch (WebSocketCpp::_(d.c_str()))
            {
                case _("start"):
                    connID = request.GetConnectionID();
                    task.Start();
                    break;
                case _("stop"):
                    task.Stop();
                    break;
                default:
                {
                    auto arr = StringUtil::Split(d, ',');
                    if (arr.size() == 2)
                    {
                        int i = 0;
                        if (StringUtil::String2int(arr[0], i))
                        {
                            min = i;
                        }
                        if (StringUtil::String2int(arr[1], i))
                        {
                            max = i;
                        }
                    }
                }
                break;
            }

            return true;
        });

        wsServer.Run();
    }
    else
    {
        WebSocketCpp::DebugPrint() << "WS server Init() failed: " << wsServer.GetLastError() << std::endl;
    }

    WebSocketCpp::DebugPrint() << "Starting... Press Ctrl-C to terminate" << std::endl;
    wsServer.WaitFor();

    return 0;
}
