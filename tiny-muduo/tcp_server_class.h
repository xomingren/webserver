#pragma once
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h> //for bzero
#include <unistd.h>

#include<vector>

#include "define.h"
#include "channel_class.h"

class TcpServer
{	
public:
	TcpServer();
	~TcpServer() = default;
	void Start();
private:
	FD epollfd_;
	FD listenfd_;
	epoll_event events_[kMaxEvents];
	static TcpServer* ptr_this;

	SocketFD CreateSocketAndListenOrDie();
	static void OnEvent(FD sockfd);
};

