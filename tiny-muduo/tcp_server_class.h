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

	void set_messagecallback(MessageCallBack messagecallback)
	{	messagecallback_ = messagecallback;	  }
	void set_connectioncallback(ConnectionCallBack connectioncallback)
	{	connectioncallback_ = connectioncallback;   }
	void Start();
	void OnNewConnection(SocketFD socketfd);
private:
	std::map<SocketFD, TcpConnection*> connections_;
	Acceptor* acceptor_;
	ConnectionCallBack connectioncallback_;
	MessageCallBack messagecallback_;
	EventLoop* loop_;
};

