#pragma once

#include "protobufcodecrpc_class.h"
#include "rpc.pb.h"

namespace detail
{
	//class Buffer;
	//class TcpConnection;
	//typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

	typedef std::shared_ptr<tiny_muduo_rpc::RpcMessage> RpcMessagePtr;
	extern const char rpctag[5];
	// wire format
	//
	// Field     Length  Content
	//
	// size      4-byte  N+8
	// "RPC0"    4-byte
	// payload   N-byte
	// checksum  4-byte  adler32 of "RPC0"+payload
	//
	typedef ProtobufCodecRpcT<tiny_muduo_rpc::RpcMessage, rpctag> RpcCodec;
}


