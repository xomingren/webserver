#include <iostream>
#include <functional>
#include <string>
#include <unistd.h>

#include "../../../tiny_muduo/commonfunction.h"
#include "../../../tiny_muduo/eventloop_class.h"
#include "../../../tiny_muduo/noncopyable_class.h"
#include "rpc_example.pb.h"
#include "../../../tiny_muduo/protorpc/rpcchannel_class.h"
#include "../../../tiny_muduo/tcpclient_class.h"
#include "../../../tiny_muduo/tcpconnection_class.h"

#include "../../../tiny_muduo/log.h"


using namespace std;
using namespace placeholders;

class RpcClient : noncopyable
{
public:
    RpcClient(EventLoop* loop)
        : loop_(loop),
          client_(loop),
          channel_(make_shared<RpcChannel>()),
          stub_(get_pointer(channel_))
    {
        client_.set_connectioncallback(
            std::bind(&RpcClient::OnConnection, this, _1));
        client_.set_messagecallback(
            std::bind(&RpcChannel::OnMessage, get_pointer(channel_), _1, _2, _3));
        // client_.enableRetry();
    }

    void Connect()
    {
        client_.Connect();
    }

private:
    void OnConnection(const TcpConnectionPtr& conn)
    {
        if (conn->Connected())
        {
            //channel_.reset(new RpcChannel(conn));
            channel_->set_connection(conn);
            rpc_example::RpcRequest request;
            for (int i = 1; i <= 5; ++i)
            {
                request.add_params(i);
            }
            rpc_example::RpcResponse* response = new rpc_example::RpcResponse;

            stub_.add(nullptr, &request, response, NewCallback(this, &RpcClient::Solved, response));
        }
        else
        {
            loop_->Quit();
        }
    }

    void Solved(rpc_example::RpcResponse* response)
    {
        cout << "solved:\n";
        if (response->solved())
        {
            cout << "result is :" << response->result() << "\n";
        }
        client_.Disconnect();
    }

    EventLoop* loop_;
    TcpClient client_;
    RpcChannelPtr channel_;
    rpc_example::RpcService_add::Stub stub_;
};

int main(int argc, char* argv[])
{
    char* curdir;
    curdir = getcwd(NULL, 0);
    tiny_muduo_log::Initialize(tiny_muduo_log::GuaranteedLogger(), string(curdir) + "/", "tinymuduolog", 1);

    LOG_INFO << "pid = " << getpid();
   
    EventLoop loop;
    RpcClient rpcclient(&loop);
    rpcclient.Connect();
    loop.Loop();
  
    google::protobuf::ShutdownProtobufLibrary();
    getchar();
}
