#include <functional>
#include <memory>

#include "../../../tiny_muduo/eventloop_class.h"
#include "../../../tiny_muduo/noncopyable_class.h"
#include "protobufcodec_class.h"
#include "protobufdispatcher_class.h"
#include "query.pb.h"
#include "../../../tiny_muduo/tcpclient_class.h"

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
          dispatcher_(std::bind(&QueryClient::OnUnknownMessage, this, _1, _2, _3)),
          codec_(std::bind(&ProtobufDispatcher::OnProtobufMessage, &dispatcher_, _1, _2, _3))
    {
        dispatcher_.RegisterMessageCallback<tiny_muduo::Answer>(
            std::bind(&QueryClient::OnAnswer, this, _1, _2, _3));
        dispatcher_.RegisterMessageCallback<tiny_muduo::Empty>(
            std::bind(&QueryClient::OnEmpty, this, _1, _2, _3));
        client_.set_connectioncallback(
            std::bind(&QueryClient::OnConnection, this, _1));
        client_.set_messagecallback(
            std::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2, _3));
    }

    void Connect()
    {
        client_.Connect();
    }

private:

    void OnConnection(const TcpConnectionPtr& conn)
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

    void OnUnknownMessage(const TcpConnectionPtr&,
        const MessagePtr& message,
        Timestamp)
    {
        cout << "OnUnknownMessage: " << message->GetTypeName();
    }

    void OnAnswer(const TcpConnectionPtr&,
        const AnswerPtr& message,
        Timestamp)
    {
        cout << "OnAnswer:\n" << message->GetTypeName() << message->DebugString();
    }

    void OnEmpty(const TcpConnectionPtr&,
        const EmptyPtr& message,
        Timestamp)
    {
        cout << "OnEmpty: " << message->GetTypeName();
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
    query.add_question("Running?");
    tiny_muduo::Empty empty;
    messageToSend = &query;

    QueryClient client(&loop);
    client.Connect();
    loop.Loop(); 
}