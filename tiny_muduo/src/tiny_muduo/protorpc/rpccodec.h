#pragma once

#include "protobufcodecrpc_class.h"

namespace tiny_muduo_rpc
{
	//class Buffer;
	//class TcpConnection;
	//using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

	class RpcMessage;
	using RpcMessagePtr = std::shared_ptr<RpcMessage>;
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
	using RpcCodec = ProtobufCodecRpcT<RpcMessage, rpctag>;
}


