#include <functional>
#include <iostream>
#include <string>
#include <unistd.h>

#include "commonfunction.h"
#include "eventloop_class.h"
#include "noncopyable_class.h"
#include "rpc_example.pb.h"
#include "rpcchannel_class.h"
#include "tcpclient_class.h"
#include "tcpconnection_class.h"

using namespace std;
using namespace placeholders;

class RpcClient : noncopyable
{
public:
    RpcClient(EventLoop* loop)
        : loop_(loop),
        client_(loop),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_))
    {
        client_.setConnectionCallback(
            std::bind(&RpcClient::OnConnection, this, _1));
        client_.setMessageCallback(
            std::bind(&RpcChannel::onMessage, get_pointer(channel_), _1, _2, _3));
        // client_.enableRetry();
    }

    void Connect()
    {
        client_.connect();
    }

private:
    void OnConnection(const TcpConnectionPtr& conn)
    {
        if (conn->Connected())
        {
            //channel_.reset(new RpcChannel(conn));
            channel_->setConnection(conn);
            rpc_example::RpcRequest request;
            request.set_callwhat("add");
            for (int i = 1; i <= 5; ++i)
            {
                request.add_params(i);
            }
            rpc_example::RpcResponse* response = new rpc_example::RpcResponse;

            stub_.Solve(NULL, &request, response, NewCallback(this, &RpcClient::solved, response));
        }
        else
        {
            loop_->Quit();
        }
    }

    void solved(rpc_example::RpcResponse* response)
    {
        cout << "solved:\n";
        if (response->solved())
        {
            cout << "result is :" << response->result() << "\n";
        }
        client_.disconnect();
    }

    EventLoop* loop_;
    TcpClient client_;
    RpcChannelPtr channel_;
    rpc_example::RpcService::Stub stub_;
};

int main(int argc, char* argv[])
{
   // cout << "pid = " << getpid();
   
    EventLoop loop;
    RpcClient rpcClient(&loop);
    rpcClient.Connect();
    loop.Loop();
  
    google::protobuf::ShutdownProtobufLibrary();
    getchar();
}
