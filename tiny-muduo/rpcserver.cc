//#include <iostream>
//#include <string>
//#include <unistd.h>
//
//#include "eventloop_class.h"
//#include "rpcserver_class.h"
//#include "rpc_example.pb.h"
//
//using namespace std;
//
//namespace rpc_example
//{
//
//    class RpcServiceImpl : public RpcService
//    {
//    public:
//        virtual void Solve(::google::protobuf::RpcController* controller,
//            const rpc_example::RpcRequest* request,
//            rpc_example::RpcResponse* response,
//            ::google::protobuf::Closure* done)
//        {
//            cout << "RpcServiceImpl::Solve...";
//            int result = 0;
//            if (request->callwhat() == add_)
//            {
//                for (int i = request->params_size(); i > 0; --i)
//                {
//                    result += request->params(i - 1);
//                }
//            }
//            if (request->callwhat() == multiplication_)
//            {
//                for (int i = request->params_size(); i > 0; --i)
//                {
//                    result *= request->params(i - 1);
//                }
//            }
//            response->set_solved(true);
//            response->set_result(result);
//            done->Run();
//        }
//    private:
//        const std::string add_ = "add";
//        const std::string multiplication_ = "multiplication";
//    };
//
//}  // namespace sudoku
//
//int main()
//{
//    //cout << "pid = " << getpid();
//    EventLoop loop;
//    rpc_example::RpcServiceImpl impl;
//    RpcServer server(&loop);
//    server.RegisterService(&impl);
//    server.Start();
//    loop.Loop();
//    google::protobuf::ShutdownProtobufLibrary();
//    getchar();
//    return 0;
//}