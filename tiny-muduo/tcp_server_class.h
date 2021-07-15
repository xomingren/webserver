#pragma once
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //for bzero
#include <iostream>
#include <unistd.h>

const uint8_t kMaxLine = 100;
const uint16_t kMaxEvents = 500;
const uint8_t kMaxListenFd = 5;

using Socket = int;
class TcpServer
{	
private:
	Socket CreateSocketAndListenOrDie();
public:
	TcpServer() = default;
	~TcpServer() = default;
	void Start();

};

