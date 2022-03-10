#pragma once

#include<sys/epoll.h>//for epoll event

#include <map>
#include <string>

#include "acceptor_class.h"
#include "channel_class.h"
#include "define.h"
#include "eventloop_class.h"
#include "tcp_connection_class.h"

class TcpServer
{	
public:
	TcpServer(EventLoop* loop);
	TcpServer(const TcpServer&) = delete;
	TcpServer& operator =(const TcpServer&) = delete;
	~TcpServer() = default;

	void set_messagecallback(MessageCallBack messagecallback)
	{ messagecallback_ = messagecallback; }
	void set_connectioncallback(ConnectionCallBack connectioncallback)
	{ connectioncallback_ = connectioncallback; }
	void set_writecompletecallback(WriteCompleteCallback cb)
	{ writecompletecallback_ = std::move(cb); }
	void set_highwatermarkcallback(HighWaterMarkCallback cb, size_t highwatermark)
	{ highwatermarkcallback_ = cb; highwatermark_ = highwatermark; }
	void Start();
	void OnNewConnection(SocketFD socketfd);

private:
	void RemoveConnection(const TcpConnectionPtr& conn);
	void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
	ConnectionMap connections_;
	Acceptor* acceptor_;
	EventLoop* loop_;
	size_t highwatermark_;
	int nextconnid_;

	ConnectionCallBack connectioncallback_;
	MessageCallBack messagecallback_;
	WriteCompleteCallback writecompletecallback_;
	HighWaterMarkCallback highwatermarkcallback_;
	
};

