#pragma once

#include<sys/epoll.h>//for epoll event

#include <map>

#include "acceptor_class.h"
#include "channel_class.h"
#include "define.h"
#include "tcp_connection _class.h"


class TcpServer
{	
public:
	TcpServer();
	~TcpServer() = default;
	void Start();
	static void OnNewConnection(SocketFD socketfd);
private:
	void update(Channel* channel,int op);

	FD epollfd_;
	epoll_event events_[kMaxEvents];
	std::map<SocketFD, TcpConnection*> connections_;
	static TcpServer* ptr_this;
	Acceptor* acceptor_;
};

