#pragma once

#include<sys/epoll.h>//for epoll event

#include <map>
#include <string>

#include "acceptor_class.h"
#include "channel_class.h"
#include "define.h"
#include "eventloop_class.h"
#include "noncopyable_class.h"
#include "tcpconnection_class.h"

class TcpServer :public noncopyable
{	
public:
	explicit TcpServer(EventLoop* loop);
	~TcpServer();

	void set_messagecallback(MessageCallback messagecallback)
	{ messagecallback_ = messagecallback; }
	void set_connectioncallback(ConnectionCallback connectioncallback)
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

	ConnectionCallback connectioncallback_;
	MessageCallback messagecallback_;
	WriteCompleteCallback writecompletecallback_;
	HighWaterMarkCallback highwatermarkcallback_;
	
};

