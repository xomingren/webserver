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
using SocketCallBack = std::function<void(FD)>;
using MessageCallBack = std::function<void(TcpConnection* connection, const std::string& data)>;
using ConnectionCallBack = std::function<void(TcpConnection* connection)>;

const uint8_t kMaxLine = 100;
const uint16_t kMaxEvents = 500;
const uint8_t kMaxListenFd = 5;
