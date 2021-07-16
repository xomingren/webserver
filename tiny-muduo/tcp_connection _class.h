#pragma once
#include "define.h"
#include "channel_class.h"

class TcpConnection
{
public:
	TcpConnection(FD epollfd, FD socketfd);
	~TcpConnection() = default;
	static void OnEvent(FD socketfd);
private:
	Channel* channel_;
	FD epollfd_;
	FD socketfd_;	
};

