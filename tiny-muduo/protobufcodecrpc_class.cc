#include "protobufcodecrpc_class.h"

#include <assert.h>
#include <zlib.h>

#include "commonfunction.h"
#include "tcpconnection_class.h"

#include <google/protobuf/message.h>

using namespace std;

void ProtobufCodecRpc::Send(const TcpConnectionPtr& conn,
    const ::google::protobuf::Message& message)
{
    // FIXME: serialize to TcpConnection::outputBuffer()
    Buffer buf;
    FillEmptyBuffer(&buf, message);
    conn->Send(&buf);
}

void ProtobufCodecRpc::FillEmptyBuffer(Buffer* buf,
    const google::protobuf::Message& message)
{
    assert(buf->ReadableBytes() == 0);
    // FIXME: can we move serialization & checksum to other thread?
    buf->Append(tag_);

    int bytesize = SerializeToBuffer(message, buf);

    int32_t checksum = Checksum(buf->Peek(), static_cast<int>(buf->ReadableBytes()));
    buf->AppendInt32(checksum);
    assert(buf->ReadableBytes() == tag_.size() + bytesize + kChecksumLen); (void)bytesize;
    int32_t len = htonl(static_cast<int32_t>(buf->ReadableBytes()));
    buf->Prepend(&len, sizeof len);
}

void ProtobufCodecRpc::OnMessage(const TcpConnectionPtr& conn,
    Buffer* buf,
    Timestamp receivetime)
{
    while (buf->ReadableBytes() >= static_cast<uint32_t>(kMinMessageLen + kHeaderLen))
    {
        const int32_t len = buf->PeekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen)
        {
            errorcallback_(conn, buf, receivetime, kInvalidLength);
            break;
        }
        else if (buf->ReadableBytes() >= implicit_cast<size_t>(kHeaderLen + len))
        {
            if (rawcb_ && !rawcb_(conn, string_view(buf->Peek(), kHeaderLen + len), receivetime))
            {
                buf->Retrieve(kHeaderLen + len);
                continue;
            }
            MessagePtr message(prototype_->New());
            // FIXME: can we move deserialization & callback to other thread?
            ErrorCode errorcode = Parse(buf->Peek() + kHeaderLen, len, message.get());
            if (errorcode == kNoError)
            {
                // FIXME: try { } catch (...) { }
                messagecallback_(conn, message, receivetime);
                buf->Retrieve(kHeaderLen + len);
            }
            else
            {
                errorcallback_(conn, buf, receivetime, errorcode);
                break;
            }
        }
        else
        {
            break;
        }
    }
}

bool ProtobufCodecRpc::ParseFromBuffer(string_view buf, google::protobuf::Message* message)
{
    return message->ParseFromArray(buf.data(), static_cast<int>(buf.size()));
}

int ProtobufCodecRpc::SerializeToBuffer(const google::protobuf::Message& message, Buffer* buf)
{
#if GOOGLE_PROTOBUF_VERSION > 3009002
    int bytesize = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
    int byte_size = message.ByteSize();
#endif
    buf->EnsureWritableBytes(bytesize + kChecksumLen);

    uint8_t* start = reinterpret_cast<uint8_t*>(buf->BeginWrite());
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    assert(end - start == bytesize); (void)start,(void)end;
    buf->HasWritten(bytesize);
    return bytesize;
}

namespace
{
    const string kNoErrorStr = "NoError";
    const string kInvalidLengthStr = "InvalidLength";
    const string kCheckSumErrorStr = "CheckSumError";
    const string kInvalidNameLenStr = "InvalidNameLen";
    const string kUnknownMessageTypeStr = "UnknownMessageType";
    const string kParseErrorStr = "ParseError";
    const string kUnknownErrorStr = "UnknownError";
}

const string& ProtobufCodecRpc::ErrorCodeToString(ErrorCode errorcode)
{
    switch (errorcode)
    {
    case kNoError:
        return kNoErrorStr;
    case kInvalidLength:
        return kInvalidLengthStr;
    case kCheckSumError:
        return kCheckSumErrorStr;
    case kInvalidNameLen:
        return kInvalidNameLenStr;
    case kUnknownMessageType:
        return kUnknownMessageTypeStr;
    case kParseError:
        return kParseErrorStr;
    default:
        return kUnknownErrorStr;
    }
}

void ProtobufCodecRpc::DefaultErrorCallback(const TcpConnectionPtr& conn,
    Buffer* buf,
    Timestamp,
    ErrorCode errorcode)
{
    cout << "ProtobufCodecLite::defaultErrorCallback - " << ErrorCodeToString(errorcode);
    if (conn && conn->Connected())
    {
        conn->Shutdown();
    }
}

int32_t ProtobufCodecRpc::AsInt32(const char* buf)
{
    int32_t be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return ntohl(be32);
}

int32_t ProtobufCodecRpc::Checksum(const void* buf, int len)
{
    return static_cast<int32_t>(
        ::adler32(1, static_cast<const Bytef*>(buf), len));
}

bool ProtobufCodecRpc::ValidateChecksum(const char* buf, int len)
{
    // check sum
    int32_t expectedchecksum = AsInt32(buf + len - kChecksumLen);
    int32_t checksum = Checksum(buf, len - kChecksumLen);
    return checksum == expectedchecksum;
}

ProtobufCodecRpc::ErrorCode ProtobufCodecRpc::Parse(const char* buf,
    int len,
    ::google::protobuf::Message* message)
{
    ErrorCode error = kNoError;

    if (ValidateChecksum(buf, len))
    {
        if (memcmp(buf, tag_.data(), tag_.size()) == 0)
        {
            // parse from buffer
            const char* data = buf + tag_.size();
            int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
            if (ParseFromBuffer(string_view(data, dataLen), message))
            {
                error = kNoError;
            }
            else
            {
                error = kParseError;
            }
        }
        else
        {
            error = kUnknownMessageType;
        }
    }
    else
    {
        error = kCheckSumError;
    }

    return error;
}