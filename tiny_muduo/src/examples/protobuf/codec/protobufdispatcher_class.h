#pragma once

#include <map>
#include <type_traits>

#include "../../../tiny_muduo/commonfunction.h"
#include "../../../tiny_muduo/define.h"
#include "../../../tiny_muduo/noncopyable_class.h"

#include <google/protobuf/message.h>

using MessagePtr = std::shared_ptr<google::protobuf::Message> ;

class Callback : noncopyable
{
public:
    virtual ~Callback() = default;

    virtual void OnMessage(const TcpConnectionPtr&,
        const MessagePtr& message,
        Timestamp) const = 0;
};

template <typename T>
class CallbackT : public Callback
{
    static_assert(std::is_base_of<google::protobuf::Message, T>::value,
        "T must be derived from gpb::Message.");
public:
    using ProtobufMessageTCallback = std::function<void(const TcpConnectionPtr&,
        const std::shared_ptr<T>& message,
        Timestamp)> ;

    CallbackT(const ProtobufMessageTCallback& callback)
        : callback_(callback)
    {
    }

    void OnMessage(const TcpConnectionPtr& conn,
        const MessagePtr& message,
        Timestamp receivetime) const override
    {
        std::shared_ptr<T> concrete = down_pointer_cast<T>(message);
        assert(concrete != nullptr);
        callback_(conn, concrete, receivetime);
    }

private:
    ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher
{
public:
    using ProtobufMessageCallback = std::function<void(const TcpConnectionPtr&,
        const MessagePtr& message,
        Timestamp)>;

    explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
        : defaultcallback_(defaultCb)
    {
    }

    void OnProtobufMessage(const TcpConnectionPtr& conn,
        const MessagePtr& message,
        Timestamp receivetime) const
    {
        CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
        if (it != callbacks_.end())
        {
            it->second->OnMessage(conn, message, receivetime);
        }
        else
        {
            defaultcallback_(conn, message, receivetime);
        }
    }

    template<typename T>
    void RegisterMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
    {
        std::shared_ptr<CallbackT<T> > pd(make_shared<CallbackT<T> >(callback));
        callbacks_[T::descriptor()] = pd;
    }

private:
    using CallbackMap = std::map<const google::protobuf::Descriptor*, std::shared_ptr<Callback> >;

    CallbackMap callbacks_;
    ProtobufMessageCallback defaultcallback_;
};
