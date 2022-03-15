#pragma once

#include <functional>

#include "define.h"
#include <memory>//for shared_ptr
#include "noncopyable_class.h"
#include "tcpconnection_class.h"

#include <google/protobuf/message.h>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }

using MessagePtr = std::shared_ptr<google::protobuf::Message>;

//
// FIXME: merge with RpcCodec
//
class ProtobufCodec : noncopyable
{
 public:
	 using ProtobufMessageCallback = std::function<void(const TcpConnectionPtr&,
		 const MessagePtr&,
		 Timestamp)>;

	/* using ErrorCallback = std::function<void(const muduo::net::TcpConnectionPtr&,
		 muduo::net::Buffer*,
		 muduo::Timestamp,
		 ErrorCode)>;*/
	 explicit ProtobufCodec(const ProtobufMessageCallback& messageCb)
		 : messagecallback_(messageCb)
		//   errorCallback_(defaultErrorCallback)
	 {
	 }

	 void OnMessage(const TcpConnectionPtr& conn,
		 Buffer* buf,
		 Timestamp receivetime);

	 void Send(const TcpConnectionPtr& conn,
		 const google::protobuf::Message& message)
	 {
		 // FIXME: serialize to TcpConnection::outputBuffer()
		 Buffer buf;
		 FillEmptyBuffer(&buf, message);
		 conn->Send(&buf);
	 }

	 static google::protobuf::Message* CreateMessage(const std::string& type);
	 static MessagePtr Parse(const char* buf, int len/*, ErrorCode* errorCode*/);

	 static int32_t AsInt32(const char* buf)
	 {
		 int32_t be32 = 0;
		 ::memcpy(&be32, buf, sizeof(be32));
		 return ntohl(be32);
	 }

	 static void FillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);
private:
	const static int kHeaderLen = sizeof(int32_t);
	const static int kMinMessageLen = 2 * kHeaderLen + 2; // nameLen + typeName + checkSum
	const static int kMaxMessageLen = 64 * 1024 * 1024; // same as codec_stream.h kDefaultTotalBytesLimit

	ProtobufMessageCallback messagecallback_;
};