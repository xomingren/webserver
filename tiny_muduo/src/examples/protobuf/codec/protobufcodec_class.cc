#include "protobufcodec_class.h"

#include <google/protobuf/descriptor.h>
#include <zlib.h>  // for adler32

#include "../../../tiny_muduo/commonfunction.h"

void ProtobufCodec::OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receivetime)
{
    while (buf->ReadableBytes() >= kMinMessageLen + kHeaderLen)
    {
        const int32_t len = buf->PeekInt32();
        if (len > kMaxMessageLen || len < kMinMessageLen)
        {
            break;
        }
        else if (buf->ReadableBytes() >= implicit_cast<size_t>(len + kHeaderLen))
        {
            MessagePtr message = Parse(buf->Peek() + kHeaderLen, len);
            messagecallback_(conn, message, receivetime);
            buf->Retrieve(kHeaderLen + len);
        }
        else
        {
            break;
        }
    }
}

google::protobuf::Message* ProtobufCodec::CreateMessage(const std::string& type)
{
    google::protobuf::Message* message = nullptr;
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type);
    if (descriptor)
    {
        const google::protobuf::Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            message = prototype->New();
        }
    }
    return message;
}

MessagePtr ProtobufCodec::Parse(const char* buf, int len)
{
    MessagePtr message;

    // check sum
    int32_t expectedchecksum = AsInt32(buf + len - kHeaderLen);
    int32_t checksum = static_cast<int32_t>(
        ::adler32(1,
            reinterpret_cast<const Bytef*>(buf),
            static_cast<int>(len - kHeaderLen)));
    if (checksum == expectedchecksum)
    {
        // get message type name
        int32_t namelen = AsInt32(buf);
        if (namelen >= 2 && namelen <= len - 2 * kHeaderLen)
        {
            std::string type(buf + kHeaderLen, buf + kHeaderLen + namelen - 1);
            // create message object
            message.reset(CreateMessage(type));
            if (message)
            {
                // parse from buffer
                const char* data = buf + kHeaderLen + namelen;
                int32_t datalen = len - namelen - 2 * kHeaderLen;
                if (message->ParseFromArray(data, datalen))
                {
                    //*error = kNoError;
                }
                else
                {
                    //*error = kParseError;
                }
            }
            else
            {
                
            }
        }
        else
        {
           
        }
    }
    else
    {
        
    }

    return message;
}

void ProtobufCodec::FillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message)
{
	assert(buf->ReadableBytes() == 0);

	const std::string& type = message.GetTypeName();
	int32_t nameLen = static_cast<int32_t>(type.size() + 1);
	buf->AppendInt32(nameLen);
	buf->Append(type.c_str(), nameLen);
	
#if GOOGLE_PROTOBUF_VERSION > 3009002
	int bytesize = google::protobuf::internal::ToIntSize(message.ByteSizeLong());
#else
	int bytesize = message.ByteSize();
#endif
	buf->EnsureWritableBytes(bytesize);
	uint8_t* start = reinterpret_cast<uint8_t*>(buf->BeginWrite());
	uint8_t* end = message.SerializeWithCachedSizesToArray(start);
	assert(end - start == bytesize);

	buf->HasWritten(bytesize);

	int32_t checksum = static_cast<int32_t>(
		::adler32(1,
			reinterpret_cast<const Bytef*>(buf->Peek()),
			static_cast<int>(buf->ReadableBytes())));
	buf->AppendInt32(checksum);
	assert(buf->ReadableBytes() == sizeof nameLen + nameLen + bytesize + sizeof checksum);
	int32_t len = htonl(static_cast<int32_t>(buf->ReadableBytes()));
	buf->Prepend(&len, sizeof len);
}
