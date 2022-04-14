#pragma once

#include<sys/epoll.h>//for epoll event

#include <map>
#include <string>

#include "define.h"
#include "noncopyable_class.h"
#include "tcpconnection_class.h"

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

class TcpServer :public noncopyable
{	
public:
	using ThreadInitCallback = std::function<void(EventLoop*)> ;

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
	void setThreadInitCallback(const ThreadInitCallback& cb)
	{ threadinitcallback_ = cb; }

	void Start();
	void OnNewConnection(SocketFD socketfd);

	void SetThreadNum(int numthreads);

private:
	void RemoveConnection(const TcpConnectionPtr& conn);
	void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

	using ConnectionMap = std::map<std::string, TcpConnectionPtr>;
	ConnectionMap connections_;
	std::unique_ptr<Acceptor> acceptor_;
	EventLoop* loop_; //acceptor loop
	size_t highwatermark_;
	int nextconnid_;
	std::shared_ptr<EventLoopThreadPool> threadpool_;//for multi-reactor
	std::atomic<int32_t> started_;

	ConnectionCallback connectioncallback_;
	MessageCallback messagecallback_;
	WriteCompleteCallback writecompletecallback_;
	HighWaterMarkCallback highwatermarkcallback_;
	ThreadInitCallback threadinitcallback_;
};

