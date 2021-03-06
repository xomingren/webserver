#include <unistd.h>

#include <functional>
#include <memory>

#include "../../../tiny_muduo/eventloop_class.h"
#include "../../../tiny_muduo/noncopyable_class.h"
#include "protobufcodec_class.h"
#include "protobufdispatcher_class.h"
#include "query.pb.h"
#include "../../../tiny_muduo/tcpserver_class.h"

#include "../../../tiny_muduo/log.h"

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
        LOG_INFO << (conn->Connected() ? "UP" : "DOWN");
    }

    void OnUnknownMessage(const TcpConnectionPtr& conn,
        const MessagePtr& message,
        Timestamp)
    {
        LOG_WARN << "OnUnknownMessage: " << message->GetTypeName();
        cout << "OnUnknownMessage: " << message->GetTypeName();
        conn->Shutdown();
    }

    void OnQuery(const TcpConnectionPtr& conn,
        const QueryPtr& message,
        Timestamp)
    {
        cout << "OnQuery:\n" << message->GetTypeName() << "\n" << message->DebugString();
        tiny_muduo::Answer answer;
        answer.set_id(1);
        answer.set_questioner(message->questioner());
        answer.set_answerer("blog.csdn.net/Solstice");
        answer.set_solution(1,"Jump!");
        answer.set_solution(2,"Win!");
        codec_.Send(conn, answer);

        conn->Shutdown();
    }

    void OnAnswer(const TcpConnectionPtr& conn,
        const AnswerPtr& message,
        Timestamp)
    {
        cout << "OnAnswer: " << message->GetTypeName();
        conn->Shutdown();
    }

    TcpServer server_;
    ProtobufDispatcher dispatcher_;
    ProtobufCodec codec_;
};

int main(int argc, char* argv[])
{
    char* curdir;
    curdir = getcwd(NULL, 0);
    tiny_muduo_log::Initialize(tiny_muduo_log::GuaranteedLogger(), string(curdir) + "/", "tinymuduolog", 1);

    LOG_INFO << "pid = " << getpid();

    EventLoop loop; 
    QueryServer server(&loop);
    server.Start();
    loop.Loop();
    return 0;
}