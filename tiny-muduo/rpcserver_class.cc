#include "rpcserver_class.h"

#include "commonfunction.h"
#include <functional>
#include <iostream>

#include "rpcchannel_class.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

using namespace std;
using namespace placeholders;

RpcServer::RpcServer(EventLoop* loop)
    : server_(loop)
{
    server_.set_connectioncallback(
        std::bind(&RpcServer::OnConnection, this, _1));
    //   server_.setMessageCallback(
    //       std::bind(&RpcServer::onMessage, this, _1, _2, _3));
}

void RpcServer::RegisterService(google::protobuf::Service* service)
{
    const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
    services_[desc->full_name()] = service;
}

void RpcServer::Start()
{
    server_.Start();
}

void RpcServer::OnConnection(const TcpConnectionPtr& conn)
{
    cout << (conn->Connected() ? "UP" : "DOWN") << endl;;
    if (conn->Connected())
    {
        RpcChannelPtr channel(make_shared <RpcChannel>(conn));
        channel->set_services(&services_);
        conn->set_messagecallback(
            std::bind(&RpcChannel::OnMessage, get_pointer(channel), _1, _2, _3));
        conn->set_context_(channel);
    }
    else
    {
        conn->set_context_(RpcChannelPtr());
        // FIXME:
    }
}
