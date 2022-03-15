#pragma once

#include <arpa/inet.h>

#include <iostream>
#include <string_view>

#include "buffer_class.h"
#include "noncopyable_class.h"
#include "tcpconnection_class.h"

class LengthHeaderCodec : noncopyable
{
public:
    using StringMessageCallback = std::function<void(const TcpConnectionPtr&,
        const std::string& message,
        Timestamp)>;

    explicit LengthHeaderCodec(const StringMessageCallback& cb)
        : messagecallback_(cb)
    {
    }

    void OnMessage(const TcpConnectionPtr& conn,
        Buffer* buf,
        Timestamp receivetime)
    {
        while (buf->ReadableBytes() >= kHeaderLen) // kHeaderLen == 4
        {
            // FIXME: use Buffer::peekInt32()
            const void* data = buf->Peek();
            int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
            const int32_t len = ntohl(be32);
            if (len > 65536 || len < 0)
            {
                std::cout << "Invalid length " << len;
                conn->Shutdown();  // FIXME: disable reading
                break;
            }
            if (buf->ReadableBytes() >= len + kHeaderLen)
            {
                buf->Retrieve(kHeaderLen);
                std::string message(buf->Peek(), len);
                messagecallback_(conn, message, receivetime);
                buf->Retrieve(len);
            }
            else
            {
                break;
            }
        }
    }

    // FIXME: TcpConnectionPtr
    void Send(TcpConnection* conn, const std::string_view& message)
    {
        Buffer buf;
        buf.Append(message.data(), message.size());
        int32_t len = static_cast<int32_t>(message.size());
        int32_t be32 = htonl(len);
        buf.Prepend(&be32, sizeof be32);
        conn->Send(&buf);
    }

private:
    StringMessageCallback messagecallback_;
    const static size_t kHeaderLen = sizeof(int32_t);
};
