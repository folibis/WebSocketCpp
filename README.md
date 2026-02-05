![example workflow](https://github.com/folibis/WebSocketCpp/actions/workflows/cmake.yml/badge.svg)

# WebSocketCpp

small and lightweight WebSocket C++11 library

*Note: the code in the midst of development, use it at your own risk*

Currently supported:
- Routing (no std::regex using) 
- Pre/post routing handlers
- Simple settings
- Simple logging
- WebSocket server (both ws and wss)
- WebSocket client

Requirements:
- Linux (Windows support coming soon)
- Gcc >= 4.8.3
- CMake
- OpenSSL & OpenSSL headers (optionally)
- zlib (optionally)

### Usage: ###

**WebSocker server**
```cpp
WebCpp::WebSocketServer wsServer;

WebCpp::Config &config = WebCpp::Config::Instance();
config.SetWsProtocol(WebCpp::Http::Protocol::WS);
config.SetWsServerPort(8081);    
    
if(wsServer.Init())
{
    wsServer.OnMessage("/ws[/{user}/]", [](const WebCpp::Request &request, WebCpp::ResponseWebSocket &response, const ByteArray &data) -> bool {
        std::string user = request.GetArg("user");
        if(user.empty())
        {
            user = "Unknown";
        }
        response.WriteText("Hello from server, " + user + "! You've sent: " + StringUtil::ByteArray2String(data));
        return true;
    });
}
wsServer.Run();
wsServer.WaitFor();

// now you can connect to the WebSocket server using ws://127.0.0.1:8081/ws or ws://127.0.0.1:8081/ws/john
// (or use included test page: examples/public/WebSocketServer.html)
```

#### WebSocket client ####
```cpp
WebCpp::WebSocketClient wsClient;
wsClient.SetOnConnect([](bool connected)
{
    std::cout << "connected: " << (connected ? "true" : "false") << std::endl;
});

wsClient.SetOnClose([]()
{
    std::cout << "closed" << std::endl;
});

wsClient.SetOnError([](const std::string &error)
{
    std::cout << "error: " << error << std::endl;
});
    
wsClient.SetOnMessage([](WebCpp::ResponseWebSocket &response)
{
        std::cout << "message received: " << StringUtil::ByteArray2String(response.GetData()) << std::endl;
});

wsClient.Init();
wsClient.Open("ws://127.0.0.1");
wsClient.SendText("Hello!");
sleep(1);
wsClient.Close();
```

**Routing**
```cpp
server.OnMessage("/(user|users)/{user:alpha}/[{action:string}/]", [&](const WebCpp::Request &request, WebCpp::ResponseWebSocket &response, const WebCpp::ByteArray &data) -> bool
{
    std::string user = request.GetArg("user");
    if(user.empty())
    {
        user = "Unknown";
    }
    
    std::string action = request.GetArg("action");
    if(action.empty())
    {
        action = "Hello!";
    }

    server.WriteText(std::string("<h2>") + user + ", " + action + "</h2>");
    server.SendResponse(response);

    return true;
});
// can be tested using URL like /user/john/hello 
// or
// /users/children/clap%20your%20hands
```
**Routing placeholders**

Placeholder | Notes | Example
------------ | ------------- | -------------
(value1\|value2) | miltiple values | /(user\|users) will work for /user, /users
{variable} | capturing variable | /user/{name} will work for /user/john and the variable can be retrieved in a handler using `request.GetArg("name")`
{variable:xxx} | variable type | xxx is one of [alpha, numeric, string, upper, lower, any], that allows to narrow down a variable type
[optional] | optional value | /user/[num] will work for /user, /user/2
\* | any value, any length | /\*.php will work for /index.php, /subfolder/index.php and whatever

**HTTPS:**

WebSocketCpp supports HTTPS requests using OpenSSL library. If you want to test that you need a private SSL key and a certificate.
You can generate a self-signed certificate using the following command:

```bash
cd ~/.ssh
openssl req -x509 -sha256 -nodes -days 365 -newkey rsa:2048 -keyout server.key -out server.cert
```
And then set the path to the certificate and the key:

```cpp
WebCpp::Config &config = WebCpp::Config::Instance();
config.SetSslSertificate("~/.ssh/server.cert");
config.SetSslKey("~/.ssh/server.key");
```






