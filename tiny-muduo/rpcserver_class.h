#pragma once

#include <map>

#include "tcpserver_class.h"

namespace google {
    namespace protobuf {

        class Service;

    }  // namespace protobuf
}  // namespace google



class RpcServer
{
    public:
        explicit RpcServer(EventLoop* loop);

        /*void setThreadNum(int numThreads)
        {
            server_.setThreadNum(numThreads);
        }*/

        void RegisterService(::google::protobuf::Service*);
        void Start();

    private:
        void OnConnection(const TcpConnectionPtr& conn);

        // void onMessage(const TcpConnectionPtr& conn,
        //                Buffer* buf,
        //                Timestamp time);

        TcpServer server_;
        std::map<std::string, ::google::protobuf::Service*> services_;
};