#pragma once

#include <bits/stdint-uintn.h>//for uint

#include <functional>//for function
#include <memory>//for shared_ptr

#include "timestamp_class.h"

//forward declear
class Buffer;
class TcpConnection;

using SocketFD = int;
using FD = int;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*,Timestamp)>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using TimerCallback = std::function<void()>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

void DefaultConnectionCallback(const TcpConnectionPtr& conn);
void DefaultMessageCallback(const TcpConnectionPtr& conn,
	Buffer* buffer,
	Timestamp receivetime);

enum class EpollStatus:char
{
	kNew = -1,
	kAdded = 1,
	kDeleted = 2,
};