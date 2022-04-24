#include <iostream>
#include <mutex>
#include <stdio.h>
#include <unistd.h>
#include <string_view>

#include "../../tiny_muduo/codec_class.h"
#include "../../tiny_muduo/commonfunction.h"
#include "../../tiny_muduo/eventloopthread_class.h"
#include "../../tiny_muduo/tcpclient_class.h"

#include "../../tiny_muduo/log.h"

using namespace std;
using namespace placeholders;

class ChatClient : noncopyable
{
public:
    ChatClient(EventLoop* loop)
        : client_(loop),
        codec_(std::bind(&ChatClient::OnStringMessage, this, _1, _2, _3))
    {
        client_.set_connectioncallback(
            std::bind(&ChatClient::onConnection, this, _1));
        client_.set_messagecallback(
            std::bind(&LengthHeaderCodec::OnMessage, &codec_, _1, _2, _3));
        client_.EnableRetry();
    }

    void connect()
    {
        client_.Connect();
    }

    void disconnect()
    {
        client_.Disconnect();
    }

    void write(const string_view& message)
    {
        unique_lock<mutex> lock(mutex_);
        if (connection_)
        {
            codec_.Send(get_pointer(connection_), message);
        }
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        LOG_INFO << (conn->Connected() ? "UP" : "DOWN");

        unique_lock<mutex> lock(mutex_);
        if (conn->Connected())
        {
            connection_ = conn;
        }
        else
        {
            connection_.reset();
        }
    }

    void OnStringMessage(const TcpConnectionPtr&,
        const string& message,
        Timestamp)
    {
        cout << message << endl;
    }

    TcpClient client_;
    LengthHeaderCodec codec_;
    mutex mutex_;
    TcpConnectionPtr connection_ GUARDED_BY(mutex_);
};

int main(int argc, char* argv[])
{
    char* curdir;
    curdir = getcwd(NULL, 0);
    tiny_muduo_log::Initialize(tiny_muduo_log::GuaranteedLogger(), string(curdir) + "/", "tinymuduolog", 1);

    LOG_INFO << "pid = " << getpid();

    EventLoopThread loopThread;

    ChatClient client(loopThread.StartLoop());
    client.connect();
    std::string line;
    while (std::getline(std::cin, line))
    {
        client.write(line);
    }
    client.disconnect();
}
      
  