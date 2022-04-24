#include <iostream>
#include <string>
#include <unistd.h>

#include "../../../tiny_muduo/eventloop_class.h"
#include "../../../tiny_muduo/protorpc/rpcserver_class.h"
#include "rpc_example.pb.h"

#include "../../../tiny_muduo/log.h"

using namespace std;

namespace rpc_example
{

    class RpcServiceImpl_add : public RpcService_add
    {
    public:
        virtual void add(::google::protobuf::RpcController* controller,
            const rpc_example::RpcRequest* request,
            rpc_example::RpcResponse* response,
            ::google::protobuf::Closure* done)
        {
            cout << "RpcService_add::Solve...";
            int result = 0;
           
            for (int i = request->params_size(); i > 0; --i)
            {
                result += request->params(i - 1);
            }
            
            response->set_solved(true);
            response->set_result(result);
            done->Run();
        }
    };

    class RpcServiceImpl_multiplication : public RpcService_multiplication
    {
    public:
        virtual void multiplication(::google::protobuf::RpcController* controller,
            const rpc_example::RpcRequest* request,
            rpc_example::RpcResponse* response,
            ::google::protobuf::Closure* done)
        {
            cout << "RpcService_multiplication::Solve...";
            int result = 0;

            for (int i = request->params_size(); i > 0; --i)
            {
                result *= request->params(i - 1);
            }

            response->set_solved(true);
            response->set_result(result);
            done->Run();
        }
    };

}  

int main()
{
    char* curdir;
    curdir = getcwd(NULL, 0);
    tiny_muduo_log::Initialize(tiny_muduo_log::GuaranteedLogger(), string(curdir) + "/", "tinymuduolog", 1);

    LOG_INFO << "pid = " << getpid();

    EventLoop loop;
    rpc_example::RpcServiceImpl_add add;
    rpc_example::RpcServiceImpl_multiplication multiplication;
    RpcServer server(&loop);
    server.RegisterService(&add);
    server.RegisterService(&multiplication);
    server.Start();
    loop.Loop();
    google::protobuf::ShutdownProtobufLibrary();
    getchar();
    return 0;
}
//rpc message data flow:
//epollwait -> channel -> tcpconnection -> rpcchannel -> rpccodec -> rpcchannel -> RpcServiceImpl