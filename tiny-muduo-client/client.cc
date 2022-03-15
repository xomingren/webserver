#include <functional>
#include <memory>

#include "eventloop_class.h"
#include "noncopyable_class.h"
#include "protobufcodec_class.h"
#include "protobufdispatcher_class.h"
#include "query.pb.h"
#include "tcpclient_class.h"

#include <stdio.h>
#include <unistd.h>

using namespace std;
using namespace std::placeholders;

typedef std::shared_ptr<tiny_muduo::Empty> EmptyPtr;
typedef std::shared_ptr<tiny_muduo::Answer> AnswerPtr;

google::protobuf::Message* messageToSend;

class QueryClient : noncopyable
{
public:
    explicit QueryClient(EventLoop* loop)
        : loop_(loop),
          client_(loop),
          dispatcher_(std::bind(&QueryClient::onUnknownMessage, this, _1, _2, _3)),
          codec_(std::bind(&ProtobufDispatcher::OnProtobufMessage, &dispatcher_, _1, _2, _3))
    {
        dispatcher_.RegisterMessageCallback<tiny_muduo::Answer>(
            std::bind(&QueryClient::onAnswer, this, _1, _2, _3));
        dispatcher_.RegisterMessageCallback<tiny_muduo::Empty>(
            std::bind(&QueryClient::onEmpty, this, _1, _2, _3));
        client_.setConnectionCallback(
            std::bind(&QueryClient::onConnection, this, _1));
        client_.setMessageCallback(
            std::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2, _3));
    }

    void connect()
    {
        client_.connect();
    }

private:

    void onConnection(const TcpConnectionPtr& conn)
    {
        cout << (conn->Connected() ? "UP" : "DOWN");

        if (conn->Connected())
        {
            codec_.Send(conn, *messageToSend);
        }
        else
        {
            loop_->Quit();
        }
    }

    void onUnknownMessage(const TcpConnectionPtr&,
        const MessagePtr& message,
        Timestamp)
    {
        cout << "onUnknownMessage: " << message->GetTypeName();
    }

    void onAnswer(const TcpConnectionPtr&,
        const AnswerPtr& message,
        Timestamp)
    {
        cout << "onAnswer:\n" << message->GetTypeName() << message->DebugString();
    }

    void onEmpty(const TcpConnectionPtr&,
        const EmptyPtr& message,
        Timestamp)
    {
        cout << "onEmpty: " << message->GetTypeName();
    }

    EventLoop* loop_;
    TcpClient client_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
    /*LOG_INFO << "pid = " << getpid();*/
    
        EventLoop loop;

        tiny_muduo::Query query;
        query.set_id(1);
        query.set_questioner("Chen Shuo");
        query.set_question("Running?");
        tiny_muduo::Empty empty;
        messageToSend = &query;

        QueryClient client(&loop);
        client.connect();
        loop.Loop();
        getchar();
   
}