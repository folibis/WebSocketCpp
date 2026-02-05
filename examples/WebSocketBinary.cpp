#include <signal.h>

#include <string>

#include "DebugPrint.h"
#include "Request.h"
#include "ResponseWebSocket.h"
#include "StringUtil.h"
#include "ThreadWorker.h"
#include "WebSocketServer.h"
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

    int              port_http     = DEFAULT_HTTP_PORT;
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
            int       num  = StringUtil::GetRand(min, max);
            ByteArray data = StringUtil::String2ByteArray(std::to_string(num));
            response.WriteText(data);
            wsServer.SendResponse(response);
            sleep(1);
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
        wsServer.OnMessage("/ws", [&](const Request& request, ResponseWebSocket& response, const ByteArray& data) -> bool {
            response.WriteBinary(data);
            return true;
        });

        wsServer.Run();
    }

    wsServer.WaitFor();

    return 0;
}
