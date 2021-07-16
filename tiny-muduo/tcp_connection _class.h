#pragma once
#include "define.h"
#include "channel_class.h"

class TcpConnection
{
public:
	TcpConnection(EventLoop* loop, FD socketfd);
	TcpConnection(const TcpConnection&) = delete;
	TcpConnection& operator =(const TcpConnection&) = delete;
	~TcpConnection() = default;

	static void OnEvent(FD socketfd);
private:
	FD socketfd_;
	EventLoop* loop_;
	Channel* channel_;	
};

