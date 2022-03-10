#pragma once

#include <bits/stdint-uintn.h>//for uint

#include <functional>//for function
#include <memory>

#include "timestamp_class.h"

//forward declear
class TcpConnection;
class EventLoop;
class Channel;
class Buffer;

using SocketFD = int;
using FD = int;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using EventCallback = std::function<void()>;
using ReadEventCallback = std::function<void(Timestamp)>;
using NewConnectionCallback = std::function<void(FD sockfd)>;
using MessageCallBack = std::function<void(const TcpConnectionPtr&, Buffer*,Timestamp)>;
using ConnectionCallBack = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;
using Functor = std::function<void()>;
using TimerCallback = std::function<void()>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;

static const uint8_t kTmpBufferLength = 100;
static const uint16_t kMaxEvents = 500;
static const uint8_t kMaxListenFd = 5;
static const uint8_t kMessageLength = 100;
static const size_t kInitialSize = 1024;
static const size_t kCheapPrepend = 8;


const char kCRLF[] = "\r\n";

enum class EpollStatus:char
{
	kNew = -1,
	kAdded = 1,
	kDeleted = 2,
};