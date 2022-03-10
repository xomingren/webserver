#include <arpa/inet.h>//for sockaddr_in
#include <unistd.h>
#include <utility>

#include <atomic>
#include <iostream>
#include <thread>

#include "current_thread_class.h"
#include "eventloop_class.h"
#include "sudoku.h"
#include "tcp_server_class.h"
#include "threadpool_class.h"

using namespace std;
using namespace placeholders;

template<typename To, typename From>
inline To implicit_cast(From const& f)
{
    return f;
}

class SudokuServer
{
public:
    const int kCells = 81;
    SudokuServer(EventLoop* loop, int numThreads)
        : server_(loop),
        numThreads_(numThreads),
        startTime_(Timestamp::Now())
    {
        server_.set_connectioncallback(
            std::bind(&SudokuServer::onConnection, this, _1));
        server_.set_messagecallback(
            std::bind(&SudokuServer::onMessage, this, _1, _2));
    }

    void start()
    {
        cout << "starting " << numThreads_ << " threads.";
        threadPool_.Start(numThreads_);
        server_.Start();
    }

private:
    void onConnection(TcpConnection* conn)
    {
        cout << "new connection";
    }

    void onMessage(TcpConnection* conn, Buffer* buf)
    {
        size_t len = buf->ReadableBytes();
        while (len >= kCells + 2)
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                string request(buf->Peek(), crlf);
                buf->RetrieveUntil(crlf + 2);
                len = buf->ReadableBytes();
                if (!ProcessRequest(conn, request))
                {
                    conn->Send("Bad Request!\r\n");
                    //conn->shutdown();//fixme
                    break;
                }
            }
            else if (len > 100) // id + ":" + kCells + "\r\n"
            {
                conn->Send("Id too long!\r\n");
                //conn->shutdown(); //fixme
                break;
            }
            else
            {
                break;
            }
        }
    }

    bool ProcessRequest(TcpConnection* conn, const string& request)
    {
        string id;
        string puzzle;
        bool goodRequest = true;

        string::const_iterator colon = find(request.begin(), request.end(), ':');
        if (colon != request.end())
        {
            id.assign(request.begin(), colon);
            puzzle.assign(colon + 1, request.end());
        }
        else
        {
            puzzle = request;
        }

        if (puzzle.size() == implicit_cast<size_t>(kCells))
        {
            threadPool_.Run(std::bind(&solve, conn, puzzle, id));
        }
        else
        {
            goodRequest = false;
        }
        return goodRequest;
    }

    static void solve(TcpConnection* conn,
        const string& puzzle,
        const string& id)
    {
        //cout << conn->name();//fix me
        string result = SolveSudoku(puzzle);
        if (id.empty())
        {
            conn->Send(result + "\r\n");
        }
        else
        {
            conn->Send(id + ":" + result + "\r\n");
        }
    }

    TcpServer server_;
    ThreadPool threadPool_;
    int numThreads_;
    Timestamp startTime_;
};

int main(int argc, char* argv[])
{
    cout << "pid = " << getpid() << ", tid = " << CurrentThread::tid();
    int numThreads = 0;
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }
    EventLoop loop;
    SudokuServer server(&loop, numThreads);
    server.start();
    loop.Loop();
}
