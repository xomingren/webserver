#pragma once
#include <string>
#include<functional>//for function
#include <bits/stdint-uintn.h>//for uint
//forward declear
class TcpConnection;
class EventLoop;
class Channel;

using SocketFD = int;
using FD = int;
using EventCallback = std::function<void()>;
using NewConnectionCallback = std::function<void(FD sockfd)>;
using MessageCallBack = std::function<void(TcpConnection* connection, std::string* data)>;
using ConnectionCallBack = std::function<void(TcpConnection* connection)>;

static const uint8_t kMaxLine = 100;
static const uint16_t kMaxEvents = 500;
static const uint8_t kMaxListenFd = 5;
static const uint8_t kMessageLength = 8;
static const char kNew = -1;
static const char kAdded = 1;