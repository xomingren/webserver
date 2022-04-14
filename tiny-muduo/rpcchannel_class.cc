#include "rpcchannel_class.h"

#include "commonfunction.h"
#include "rpc.pb.h"

#include <google/protobuf/descriptor.h>

using namespace std;
using namespace placeholders;
using namespace detail;
using namespace tiny_muduo_rpc;

RpcChannel::RpcChannel()
    : codec_(std::bind(&RpcChannel::OnRpcMessage, this, _1, _2, _3)),
      services_(nullptr)
{
    cout << "RpcChannel::ctor - " << this;
}

RpcChannel::RpcChannel(const TcpConnectionPtr& conn)
    : codec_(std::bind(&RpcChannel::OnRpcMessage, this, _1, _2, _3)),
      connection_(conn),
      services_(nullptr)
{
    cout << "RpcChannel::ctor - " << this;
}

RpcChannel::~RpcChannel()
{
    cout << "RpcChannel::dtor - " << this;
    for (const auto& outstanding : outstandings_)
    {
        OutstandingCall out = outstanding.second;
        delete out.response;
        delete out.done;
    }
}

// Call the given method of the remote service.  The signature of this
// procedure looks the same as Service::CallMethod(), but the requirements
// are less strict in one important way:  the request and response objects
// need not be of any specific class as long as their descriptors are
// method->input_type() and method->output_type().
void RpcChannel::CallMethod(const ::google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const ::google::protobuf::Message* request,
    ::google::protobuf::Message* response,
    ::google::protobuf::Closure* done)
{
    tiny_muduo_rpc::RpcMessage message;
    message.set_type(tiny_muduo_rpc::REQUEST);
    int64_t id = ++id_;
    message.set_id(id);
    message.set_service(method->service()->full_name());
    message.set_method(method->name());
    message.set_request(request->SerializeAsString()); // FIXME: error check

    OutstandingCall out = { response, done };
    {
        lock_guard<mutex> lock(mutex_);
        outstandings_[id] = out;
    }
    codec_.Send(connection_, message);
}

void RpcChannel::OnMessage(const TcpConnectionPtr& conn,
    Buffer* buf,
    Timestamp receiveTime)
{
    codec_.OnMessage(conn, buf, receiveTime);
}

void RpcChannel::OnRpcMessage(const TcpConnectionPtr& conn,
    const RpcMessagePtr& messagePtr,
    Timestamp receiveTime)
{
    assert(conn == connection_);
    //printf("%s\n", message.DebugString().c_str());
    tiny_muduo_rpc::RpcMessage& message = *messagePtr;
    if (message.type() == RESPONSE)
    {
        int64_t id = message.id();
        assert(message.has_response() || message.has_error());

        OutstandingCall out = { nullptr, nullptr };

        {
            lock_guard<mutex> lock(mutex_);
            std::map<int64_t, OutstandingCall>::iterator it = outstandings_.find(id);
            if (it != outstandings_.end())
            {
                out = it->second;
                outstandings_.erase(it);
            }
        }

        if (out.response)
        {
            std::unique_ptr<google::protobuf::Message> d(out.response);
            if (message.has_response())
            {
                out.response->ParseFromString(message.response());
            }
            if (out.done)
            {
                out.done->Run();
            }
        }
    }
    else if (message.type() == REQUEST)
    {
        // FIXME: extract to a function
        ErrorCode error = WRONG_PROTO;
        if (services_)
        {
            std::map<std::string, google::protobuf::Service*>::const_iterator it = services_->find(message.service());
            if (it != services_->end())
            {
                google::protobuf::Service* service = it->second;
                assert(service != nullptr);
                const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
                const google::protobuf::MethodDescriptor* method
                    = desc->FindMethodByName(message.method());
                if (method)
                {
                    std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
                    if (request->ParseFromString(message.request()))
                    {
                        google::protobuf::Message* response = service->GetResponsePrototype(method).New();
                        // response is deleted in doneCallback
                        int64_t id = message.id();
                        service->CallMethod(method, nullptr, get_pointer(request), response,
                            NewCallback(this, &RpcChannel::DoneCallback, response, id));
                        error = NO_ERROR;
                    }
                    else
                    {
                        error = INVALID_REQUEST;
                    }
                }
                else
                {
                    error = NO_METHOD;
                }
            }
            else
            {
                error = NO_SERVICE;
            }
        }
        else
        {
            error = NO_SERVICE;
        }
        if (error != NO_ERROR)
        {
            tiny_muduo_rpc::RpcMessage response;
            response.set_type(RESPONSE);
            response.set_id(message.id());
            response.set_error(error);
            codec_.Send(connection_, response);
        }
    }
    else if (message.type() == ERROR)
    {
    }
}

void RpcChannel::DoneCallback(::google::protobuf::Message* response, int64_t id)
{
    std::unique_ptr<google::protobuf::Message> d(response);
    tiny_muduo_rpc::RpcMessage message;
    message.set_type(RESPONSE);
    message.set_id(id);
    message.set_response(response->SerializeAsString()); // FIXME: error check
    codec_.Send(connection_, message);
}
