#include <stdio.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <memory>

#include "eventloop_class.h"
#include "noncopyable_class.h"
#include "protobufcodec_class.h"
#include "protobufdispatcher_class.h"
#include "query.pb.h"
#include "tcpserver_class.h"

using namespace std;

using QueryPtr = std::shared_ptr<tiny_muduo::Query>;
using AnswerPtr = std::shared_ptr<tiny_muduo::Answer>;

class QueryServer : noncopyable
{
public:
    explicit QueryServer(EventLoop* loop)
        : server_(loop),
          dispatcher_(std::bind(&QueryServer::OnUnknownMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
          codec_(std::bind(&ProtobufDispatcher::OnProtobufMessage, &dispatcher_, placeholders::_1, placeholders::_2, placeholders::_3))
    {
        dispatcher_.RegisterMessageCallback<tiny_muduo::Query>(
            std::bind(&QueryServer::OnQuery, this, placeholders::_1, placeholders::_2, placeholders::_3));
        dispatcher_.RegisterMessageCallback<tiny_muduo::Answer>(
            std::bind(&QueryServer::OnAnswer, this, placeholders::_1, placeholders::_2, placeholders::_3));
        server_.set_connectioncallback(
            std::bind(&QueryServer::OnConnection, this, placeholders::_1));
        server_.set_messagecallback(
            std::bind(&ProtobufCodec::OnMessage, &codec_, placeholders::_1, placeholders::_2, placeholders::_3));
    }

    void Start()
    {
        server_.Start();
    }

private:
    void OnConnection(const TcpConnectionPtr& conn)
    {
        cout << (conn->Connected() ? "UP" : "DOWN");
    }

    void OnUnknownMessage(const TcpConnectionPtr& conn,
        const MessagePtr& message,
        Timestamp)
    {
        cout << "onUnknownMessage: " << message->GetTypeName();
        conn->Shutdown();
    }

    void OnQuery(const TcpConnectionPtr& conn,
        const QueryPtr& message,
        Timestamp)
    {
        cout << "onQuery:\n" << message->GetTypeName() << message->DebugString();
        tiny_muduo::Answer answer;
        answer.set_id(1);
        answer.set_questioner("Chen Shuo");
        answer.set_answerer("blog.csdn.net/Solstice");
        answer.set_solution("Jump!");
        answer.set_solution("Win!");
        codec_.Send(conn, answer);

        conn->Shutdown();
    }

    void OnAnswer(const TcpConnectionPtr& conn,
        const AnswerPtr& message,
        Timestamp)
    {
        cout << "onAnswer: " << message->GetTypeName();
        conn->Shutdown();
    }

    TcpServer server_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

//int main(int argc, char* argv[])
//{
//    /*LOG_INFO << "pid = " << getpid();*/
//    EventLoop loop; 
//    QueryServer server(&loop);
//    server.Start();
//    loop.Loop();
//    return 0;
//}