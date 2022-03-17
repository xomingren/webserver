#pragma once

#include "protobufcodecrpc_class.h"


class RpcMessage;
typedef std::shared_ptr<RpcMessage> RpcMessagePtr;
const char rpctag[] = "RPC0";
// wire format
//
// Field     Length  Content
//
// size      4-byte  N+8
// "RPC0"    4-byte
// payload   N-byte
// checksum  4-byte  adler32 of "RPC0"+payload
//
typedef ProtobufCodecRpcT<RpcMessage, rpctag> RpcCodec;

