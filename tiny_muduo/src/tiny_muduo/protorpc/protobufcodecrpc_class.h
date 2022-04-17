#pragma once

// For Protobuf codec supporting multiple message types, check
// examples/protobuf/codec

#include <functional>
#include <memory>
#include <string_view>
#include <type_traits>

#include "../define.h"
#include "../noncopyable_class.h"
#include "../timestamp_class.h"

namespace google
{
    namespace protobuf
    {
        class Message;
    }
}

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

// wire format
//
// Field     Length  Content
//
// size      4-byte  M+N+4
// tag       M-byte  could be "RPC0", etc.
// payload   N-byte
// checksum  4-byte  adler32 of tag+payload
//
// This is an internal class, you should use ProtobufCodecT instead.
class ProtobufCodecRpc : noncopyable
{
public:
    const static int kHeaderLen = sizeof(int32_t);
    const static int kChecksumLen = sizeof(int32_t);
    const static int kMaxMessageLen = 64 * 1024 * 1024; // same as codec_stream.h kDefaultTotalBytesLimit

    enum ErrorCode
    {
        kNoError = 0,
        kInvalidLength,
        kCheckSumError,
        kInvalidNameLen,
        kUnknownMessageType,
        kParseError,
    };

    // return false to stop parsing protobuf message
    using RawMessageCallback = std::function<bool(const TcpConnectionPtr&,
                                                  std::string_view,
                                                  Timestamp)> ;

    using ProtobufMessageCallback = std::function<void(const TcpConnectionPtr&,
                                                       const MessagePtr&,
                                                       Timestamp)>;

    using ErrorCallback = std::function<void(const TcpConnectionPtr&,
                                             Buffer*,
                                             Timestamp,
                                             ErrorCode)>;

    ProtobufCodecRpc(const ::google::protobuf::Message* prototype,
                     std::string_view tag,
                     const ProtobufMessageCallback& messagecb,
                     const RawMessageCallback& rawcb = RawMessageCallback(),
                     const ErrorCallback& errorcb = DefaultErrorCallback)
        : prototype_(prototype),
          tag_(tag),
          messagecallback_(messagecb),
          rawcb_(rawcb),
          errorcallback_(errorcb),
          kMinMessageLen(static_cast<int>(tag.size() + kChecksumLen))
    {
    }

    virtual ~ProtobufCodecRpc() = default;

    const std::string& get_tag() const { return tag_; }

    void Send(const TcpConnectionPtr& conn,
              const ::google::protobuf::Message& message);

    void OnMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receivetime);

    virtual bool ParseFromBuffer(std::string_view buf, google::protobuf::Message* message);
    virtual int SerializeToBuffer(const google::protobuf::Message& message, Buffer* buf);

    static const std::string& ErrorCodeToString(ErrorCode errorcode);

    // public for unit tests
    ErrorCode Parse(const char* buf, int len, ::google::protobuf::Message* message);
    void FillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);

    static int32_t Checksum(const void* buf, int len);
    static bool ValidateChecksum(const char* buf, int len);
    static int32_t AsInt32(const char* buf);
    static void DefaultErrorCallback(const TcpConnectionPtr&,
                                     Buffer*,
                                     Timestamp,
                                     ErrorCode);

private:
    const ::google::protobuf::Message* prototype_;
    const std::string tag_;
    ProtobufMessageCallback messagecallback_;
    RawMessageCallback rawcb_;
    ErrorCallback errorcallback_;
    const int kMinMessageLen;
};

template<typename MSG, const char* TAG, typename CODEC = ProtobufCodecRpc>  // TAG must be a variable with external linkage, not a string literal
class ProtobufCodecRpcT
{
    static_assert(std::is_base_of<ProtobufCodecRpc, CODEC>::value, "CODEC should be derived from ProtobufCodecRpc");
public:
    using ConcreteMessagePtr = std::shared_ptr<MSG> ;
    using ProtobufMessageCallback = std::function<void(const TcpConnectionPtr&,
                                                       const ConcreteMessagePtr&,
                                                       Timestamp)> ;
    using RawMessageCallback = ProtobufCodecRpc::RawMessageCallback;
    using ErrorCallback = ProtobufCodecRpc::ErrorCallback;

    explicit ProtobufCodecRpcT(const ProtobufMessageCallback& messagecb,
                               const RawMessageCallback& rawcb = RawMessageCallback(),
                               const ErrorCallback& errorcb = ProtobufCodecRpc::DefaultErrorCallback)
        : messagecallback_(messagecb),
          codec_(&MSG::default_instance(),
                 TAG,
                 std::bind(&ProtobufCodecRpcT::OnRpcMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                 rawcb,
                 errorcb)
    {
    }

    const std::string& get_tag() const { return codec_.tag(); }

    void Send(const TcpConnectionPtr& conn,
              const MSG& message)
    {
        codec_.Send(conn, message);
    }

    void OnMessage(const TcpConnectionPtr& conn,
                   Buffer* buf,
                   Timestamp receivetime)
    {
        codec_.OnMessage(conn, buf, receivetime);
    }

    // internal
    void OnRpcMessage(const TcpConnectionPtr& conn,
                      const MessagePtr& message,
                      Timestamp receivetime)
    {
        messagecallback_(conn, down_pointer_cast<MSG>(message), receivetime);
    }

    void FillEmptyBuffer(Buffer* buf, const MSG& message)
    {
        codec_.FillEmptyBuffer(buf, message);
    }

private:
    ProtobufMessageCallback messagecallback_;
    CODEC codec_;
};