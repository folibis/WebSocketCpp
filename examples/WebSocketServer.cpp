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
 * Included HTTP server provides a simple test page.
*/


#include <csignal>
#include "common.h"
#include "WebSocketServer.h"
#include "Request.h"
#include "ResponseWebSocket.h"
#include "StringUtil.h"
#include "DebugPrint.h"
#include "ThreadWorker.h"
#include "example_common.h"
#include "Platform.h"

using namespace WebCpp;
static WebCpp::WebSocketServer *wsServerPtr = nullptr;

void handle_sigint(int)
{
    wsServerPtr->Close(false);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, handle_sigint);

    int port_ws = DEFAULT_WS_PORT;
    WebCpp::Protocol http_protocol = DEFAULT_HTTP_PROTOCOL;
    WebCpp::Protocol ws_protocol = DEFAULT_WS_PROTOCOL;

    auto cmdline = CommandLine::Parse(argc, argv);

    if(cmdline.Exists("-h"))
    {
        cmdline.PrintUsage(true, true);
        exit(0);
    }

    if(cmdline.Exists("-v"))
    {
        WebCpp::DebugPrint::AllowPrint = true;
    }

    int v;
    if(StringUtil::String2int(cmdline.Get("-p"), v))
    {
        port_ws = v;
    }

    std::string s;
    if(cmdline.Set("-r", s) == true)
    {
        ws_protocol = WebCpp::String2Protocol(s);
    }

    int connID = (-1);
    int min = 1,max = 100;
    WebCpp::WebSocketServer wsServer;
    wsServerPtr = &wsServer;
    WebCpp::ThreadWorker task;
    task.SetFunction([&](bool &running) -> void*
    {
        WebCpp::ResponseWebSocket response(connID);
        StringUtil::RandInit();
        while(running)
        {
            int num = StringUtil::GetRand(min, max);
            WebCpp::ByteArray data = StringUtil::String2ByteArray(std::to_string(num));
            response.WriteText(data);
            wsServer.SendResponse(response);
            WebCpp::Sleep(1);
        }

        return nullptr;
    });

    WebCpp::Config &config = WebCpp::Config::Instance();
    config.SetRoot(PUB);
    config.SetWsProtocol(ws_protocol);
    config.SetWsServerPort(port_ws);
    config.SetSslSertificate(SSL_CERT);
    config.SetSslKey(SSL_KEY);

    if(wsServer.Init())
    {
        wsServer.OnMessage("/ws", [&](const WebCpp::Request &request, WebCpp::ResponseWebSocket &response, const WebCpp::ByteArray &data) -> bool
        {
            std::string d = StringUtil::ByteArray2String(data);
            switch(WebCpp::_(d.c_str()))
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
                        auto arr = StringUtil::Split(d,',');
                        if(arr.size() == 2)
                        {
                            int i = 0;
                            if(StringUtil::String2int(arr[0],i))
                            {
                                min = i;
                            }
                            if(StringUtil::String2int(arr[1],i))
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
        WebCpp::DebugPrint() << "WS server Init() failed: " << wsServer.GetLastError() << std::endl;
    }

    WebCpp::DebugPrint() << "Starting... Press Ctrl-C to terminate" << std::endl;
    wsServer.WaitFor();

    return 0;
}

