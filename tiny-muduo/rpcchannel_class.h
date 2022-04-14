#pragma once

#include <atomic>
#include <map>
#include <mutex>

#include "protobufcodecrpc_class.h"
#include "rpccodec.h"
#include "thread_annotations.h"

#include <google/protobuf/service.h>

namespace google {
    namespace protobuf {

        // Defined in other files.
        class Descriptor;            // descriptor.h
        class ServiceDescriptor;     // descriptor.h
        class MethodDescriptor;      // descriptor.h
        class Message;               // message.h

        class Closure;

        class RpcController;
        class Service;

    }  // namespace protobuf
}  // namespace google

    // Abstract interface for an RPC channel.  An RpcChannel represents a
    // communication line to a Service which can be used to call that Service's
    // methods.  The Service may be running on another machine.  Normally, you
    // should not call an RpcChannel directly, but instead construct a stub Service
    // wrapping it.  Example:
    // FIXME: update here
    //   RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
    //   MyService* service = new MyService::Stub(channel);
    //   service->MyMethod(request, &response, callback);

class RpcChannel : public ::google::protobuf::RpcChannel
{
public:
    RpcChannel();

    explicit RpcChannel(const TcpConnectionPtr& conn);

    ~RpcChannel() override;

    void set_connection(const TcpConnectionPtr& conn)
    {
        connection_ = conn;
    }

    void set_services(const std::map<std::string, ::google::protobuf::Service*>* services)
    {
        services_ = services;
    }

    // Call the given method of the remote service.  The signature of this
    // procedure looks the same as Service::CallMethod(), but the requirements
    // are less strict in one important way:  the request and response objects
    // need not be of any specific class as long as their descriptors are
    // method->input_type() and method->output_type().
    void CallMethod(const ::google::protobuf::MethodDescriptor* method,
                    ::google::protobuf::RpcController* controller,
                    const ::google::protobuf::Message* request,
                    ::google::protobuf::Message* response,
                    ::google::protobuf::Closure* done) override;

    void OnMessage(const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp receivetime);

private:
    void OnRpcMessage(const TcpConnectionPtr& conn,
        const detail::RpcMessagePtr& messagePtr,
        Timestamp receivetime);

    void DoneCallback(::google::protobuf::Message* response, int64_t id);

    struct OutstandingCall
    {
        ::google::protobuf::Message* response;
        ::google::protobuf::Closure* done;
    };

    detail::RpcCodec codec_;
    TcpConnectionPtr connection_;
    std::atomic<int64_t> id_;

    std::mutex mutex_;
    std::map<int64_t, OutstandingCall> outstandings_ GUARDED_BY(mutex_);

    const std::map<std::string, ::google::protobuf::Service*>* services_;
};
using RpcChannelPtr = std::shared_ptr<RpcChannel>;