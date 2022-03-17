#include "protobufcodecrpc_class.h"

#include <assert.h>
#include <zlib.h>

#include "commonfunction.h"
#include "tcpconnection_class.h"

#include <google/protobuf/message.h>

using namespace std;

void ProtobufCodecRpc::send(const TcpConnectionPtr& conn,
    const ::google::protobuf::Message& message)
{
    // FIXME: serialize to TcpConnection::outputBuffer()
    Buffer buf;
    fillEmptyBuffer(&buf, message);
    conn->Send(&buf);
}

void ProtobufCodecRpc::fillEmptyBuffer(Buffer* buf,
    const google::protobuf::Message& message)
{
    assert(buf->ReadableBytes() == 0);
    // FIXME: can we move serialization & checksum to other thread?
    buf->Append(tag_);

    int byte_size = serializeToBuffer(message, buf);

    int32_t checkSum = checksum(buf->Peek(), static_cast<int>(buf->ReadableBytes()));
    buf->AppendInt32(checkSum);
    assert(buf->ReadableBytes() == tag_.size() + byte_size + kChecksumLen); (void)byte_size;
    int32_t len = htonl(static_cast<int32_t>(buf->ReadableBytes()));
    buf->Prepend(&len, sizeof len);
}

void ProtobufCodecRpc::onMessage(const TcpConnectionPtr& conn,
    Buffer* buf,
    Timestamp receiveTime)
{
    while (buf->ReadableBytes() >= static_cast<uint32_t>(kMinMessageLen + kHeaderLen))
    {
        const int32_t len = buf->PeekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen)
        {
            errorCallback_(conn, buf, receiveTime, kInvalidLength);
            break;
        }
        else if (buf->ReadableBytes() >= implicit_cast<size_t>(kHeaderLen + len))
        {
            if (rawCb_ && !rawCb_(conn, string_view(buf->Peek(), kHeaderLen + len), receiveTime))
            {
                buf->Retrieve(kHeaderLen + len);
                continue;
            }
            MessagePtr message(prototype_->New());
            // FIXME: can we move deserialization & callback to other thread?
            ErrorCode errorCode = parse(buf->Peek() + kHeaderLen, len, message.get());
            if (errorCode == kNoError)
            {
                // FIXME: try { } catch (...) { }
                messageCallback_(conn, message, receiveTime);
                buf->Retrieve(kHeaderLen + len);
            }
            else
            {
                errorCallback_(conn, buf, receiveTime, errorCode);
                break;
            }
        }
        else
        {
            break;
        }
    }
}

bool ProtobufCodecRpc::parseFromBuffer(string_view buf, google::protobuf::Message* message)
{
    return message->ParseFromArray(buf.data(), static_cast<int>(buf.size()));
}

int ProtobufCodecRpc::serializeToBuffer(const google::protobuf::Message& message, Buffer* buf)
{
#if GOOGLE_PROTOBUF_VERSION > 3009002
    int byte_size = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
    int byte_size = message.ByteSize();
#endif
    buf->EnsureWritableBytes(byte_size + kChecksumLen);

    uint8_t* start = reinterpret_cast<uint8_t*>(buf->BeginWrite());
    uint8_t* end = message.SerializeWithCachedSizesToArray(start);
    assert(end - start == byte_size);
    buf->HasWritten(byte_size);
    return byte_size;
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

const string& ProtobufCodecRpc::errorCodeToString(ErrorCode errorCode)
{
    switch (errorCode)
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

void ProtobufCodecRpc::defaultErrorCallback(const TcpConnectionPtr& conn,
    Buffer* buf,
    Timestamp,
    ErrorCode errorCode)
{
    cout << "ProtobufCodecLite::defaultErrorCallback - " << errorCodeToString(errorCode);
    if (conn && conn->Connected())
    {
        conn->Shutdown();
    }
}

int32_t ProtobufCodecRpc::asInt32(const char* buf)
{
    int32_t be32 = 0;
    ::memcpy(&be32, buf, sizeof(be32));
    return ntohl(be32);
}

int32_t ProtobufCodecRpc::checksum(const void* buf, int len)
{
    return static_cast<int32_t>(
        ::adler32(1, static_cast<const Bytef*>(buf), len));
}

bool ProtobufCodecRpc::validateChecksum(const char* buf, int len)
{
    // check sum
    int32_t expectedCheckSum = asInt32(buf + len - kChecksumLen);
    int32_t checkSum = checksum(buf, len - kChecksumLen);
    return checkSum == expectedCheckSum;
}

ProtobufCodecRpc::ErrorCode ProtobufCodecRpc::parse(const char* buf,
    int len,
    ::google::protobuf::Message* message)
{
    ErrorCode error = kNoError;

    if (validateChecksum(buf, len))
    {
        if (memcmp(buf, tag_.data(), tag_.size()) == 0)
        {
            // parse from buffer
            const char* data = buf + tag_.size();
            int32_t dataLen = len - kChecksumLen - static_cast<int>(tag_.size());
            if (parseFromBuffer(string_view(data, dataLen), message))
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