#pragma once

#include<sys/epoll.h>//for epoll event

#include <map>

#include "acceptor_class.h"
#include "channel_class.h"
#include "define.h"
#include "eventloop _class.h"
#include "tcp_connection _class.h"


class TcpServer
{	
public:
	TcpServer(EventLoop* loop);
	TcpServer(const TcpServer&) = delete;
	TcpServer& operator =(const TcpServer&) = delete;
	~TcpServer() = default;

	void Start();
	static void OnNewConnection(SocketFD socketfd);
private:
	std::map<SocketFD, TcpConnection*> connections_;
	static TcpServer* ptr_this;
	Acceptor* acceptor_;
	EventLoop* loop_;
};

