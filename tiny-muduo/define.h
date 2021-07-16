#pragma once
#include<functional>//for function
#include <bits/stdint-uintn.h>//for uint

using SocketFD = int;
using FD = int;
using CallBackFunc = std::function<void(FD)>;

const uint8_t kMaxLine = 100;
const uint16_t kMaxEvents = 500;
const uint8_t kMaxListenFd = 5;
