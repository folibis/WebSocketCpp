/*
 *
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
 *
 */

/*
 * TicTacToe - a two-player tic-tac-toe game server.
 * Open TicTacToe.html in two browser tabs to play.
 *
 */

#include <array>
#include <csignal>
#include <mutex>
#include <string>

#include "DebugPrint.h"
#include "Request.h"
#include "ResponseWebSocket.h"
#include "StringUtil.h"
#include "WebSocketServer.h"
#include "common.h"
#include "example_common.h"

using namespace WebSocketCpp;

static WebSocketServer* wsServerPtr = nullptr;

void handle_sigint(int)
{
    wsServerPtr->Close(false);
}

struct Game
{
    std::array<char, 9> board;
    int                 connX{-1};
    int                 connO{-1};
    char                turn{'X'};
    bool                over{false};

    Game()
    {
        board.fill('.');
    }

    void resetBoard()
    {
        board.fill('.');
        turn = 'X';
        over = false;
    }

    char mark(int connID) const
    {
        if (connID == connX)
            return 'X';
        if (connID == connO)
            return 'O';
        return '\0';
    }

    bool full() const
    {
        return connX != -1 && connO != -1;
    }

    std::string boardStr() const
    {
        return {board.begin(), board.end()};
    }

    char checkResult() const
    {
        static const int lines[8][3] = {
            {0, 1, 2}, {3, 4, 5}, {6, 7, 8},
            {0, 3, 6}, {1, 4, 7}, {2, 5, 8},
            {0, 4, 8}, {2, 4, 6}};
        for (const auto& line : lines)
        {
            char c = board[line[0]];
            if (c != '.' && c == board[line[1]] && c == board[line[2]])
                return c;
        }
        for (char c : board)
        {
            if (c == '.')
                return '\0';
        }

        return 'D';
    }
};

static void sendMsg(WebSocketServer& srv, int connID, const std::string& msg)
{
    if (connID == -1)
    {
        return;
    }

    ResponseWebSocket resp(connID);
    resp.WriteText(StringUtil::String2ByteArray(msg));
    srv.SendResponse(resp);
}

static void sendBoth(WebSocketServer& srv, const Game& g, const std::string& msg)
{
    sendMsg(srv, g.connX, msg);
    sendMsg(srv, g.connO, msg);
}

static std::string startMsg(char yourMark, const std::string& board, char turn)
{
    return std::string("{\"event\":\"start\",\"mark\":\"") + yourMark + "\",\"board\":\"" + board + "\",\"turn\":\"" + turn + "\"}";
}

static std::string moveMsg(const std::string& board, char turn)
{
    return std::string("{\"event\":\"move\",\"board\":\"") + board + "\",\"turn\":\"" + turn + "\"}";
}

static std::string gameoverMsg(const std::string& board, char result)
{
    if (result == 'D')
        return std::string("{\"event\":\"gameover\",\"board\":\"") + board + "\",\"result\":\"draw\"}";
    return std::string("{\"event\":\"gameover\",\"board\":\"") + board + "\",\"result\":\"win\",\"winner\":\"" + result + "\"}";
}

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_sigint);

    auto cmdline = CommandLine::Parse(argc, argv);
    if (cmdline.Exists("-h"))
    {
        cmdline.PrintUsage(true);
        return 0;
    }
    if (cmdline.Exists("-v"))
        DebugPrint::AllowPrint = true;

    int port = DEFAULT_WS_PORT;
    int v;
    if (StringUtil::String2int(cmdline.Get("-p"), v))
        port = v;

    Config& config = Config::Instance();
    config.SetWsProtocol(Protocol::WS);
    config.SetWsServerPort(port);

    Game       game;
    std::mutex gameMutex;

    WebSocketServer wsServer;
    wsServerPtr = &wsServer;

    if (!wsServer.Init())
    {
        DebugPrint() << "Init failed: " << wsServer.GetLastError() << std::endl;
        return 1;
    }

    wsServer.OnConnect([](const Request& request) {
        DebugPrint() << "client connected: " << request.GetConnectionID() << std::endl;
    });

    wsServer.OnDisconnect([&](const Request& request) {
        std::lock_guard<std::mutex> lock(gameMutex);
        int                         connID = request.GetConnectionID();

        DebugPrint() << "client disconnected: " << connID << std::endl;

        if (connID != game.connX && connID != game.connO)
        {
            return;
        }

        int other = (connID == game.connX) ? game.connO : game.connX;
        game.resetBoard();

        if (other != -1)
        {
            game.connX = other;
            game.connO = -1;
            sendMsg(wsServer, other, R"({"event":"wait"})");
        }
        else
        {
            game.connX = game.connO = -1;
        }
    });

    wsServer.OnMessage("/tictactoe", [&](const Request& request, ResponseWebSocket& /*response*/, const ByteArray& data) -> bool {
        std::lock_guard<std::mutex> lock(gameMutex);
        int                         connID = request.GetConnectionID();
        std::string                 msg    = StringUtil::ByteArray2String(data);

        if (msg == "join")
        {
            if (game.connX == -1)
            {
                game.connX = connID;
                sendMsg(wsServer, connID, R"({"event":"wait"})");
                DebugPrint() << "player X joined: " << connID << std::endl;
            }
            else if (game.connO == -1 && connID != game.connX)
            {
                game.connO        = connID;
                std::string board = game.boardStr();
                sendMsg(wsServer, game.connX, startMsg('X', board, 'X'));
                sendMsg(wsServer, game.connO, startMsg('O', board, 'X'));
                DebugPrint() << "player O joined: " << connID << " - game started" << std::endl;
            }
            else
            {
                sendMsg(wsServer, connID, R"({"event":"full"})");
                DebugPrint() << "rejected (game full): " << connID << std::endl;
            }
            return true;
        }

        if (msg == "restart")
        {
            if (!game.full())
            {
                return true;
            }
            game.resetBoard();
            std::string board = game.boardStr();
            sendMsg(wsServer, game.connX, startMsg('X', board, 'X'));
            sendMsg(wsServer, game.connO, startMsg('O', board, 'X'));
            DebugPrint() << "game restarted" << std::endl;
            return true;
        }

        if (!game.full())
        {
            return true;
        }

        char playerMark = game.mark(connID);
        if (playerMark == '\0' || game.over || playerMark != game.turn)
        {
            return true;
        }

        if (msg.size() > 5 && msg.compare(0, 5, "move:") == 0)
        {
            int cell = -1;
            StringUtil::String2int(msg.substr(5), cell);
            if (cell < 0 || cell > 8 || game.board[cell] != '.')
            {
                return true;
            }

            game.board[cell]   = playerMark;
            char        result = game.checkResult();
            std::string board  = game.boardStr();

            DebugPrint() << "move: " << playerMark << " -> cell " << cell << "  board: " << board << std::endl;

            if (result != '\0')
            {
                game.over = true;
                sendBoth(wsServer, game, gameoverMsg(board, result));
            }
            else
            {
                game.turn = (playerMark == 'X') ? 'O' : 'X';
                sendBoth(wsServer, game, moveMsg(board, game.turn));
            }
        }

        return true;
    });

    wsServer.Run();
    DebugPrint() << "TicTacToe server on port " << port << " - open TicTacToe.html in two tabs" << std::endl;
    wsServer.WaitFor();

    return 0;
}
