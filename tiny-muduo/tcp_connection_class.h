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

	void OnRecieve();
	void OnWrite();
	void OnConnectEstablished();

	void Send(const std::string& message);
	
	void set_messagecallback(MessageCallBack messagecallback)
	{	messagecallback_ = messagecallback;	  }
	void set_connectioncallback(ConnectionCallBack connectioncallback)
	{	connectioncallback_ = connectioncallback;	}
private:
	FD socketfd_;
	EventLoop* loop_;
	Channel* channel_;
	std::string* inbuf_;
	std::string* outbuf_;
	MessageCallBack messagecallback_;
	ConnectionCallBack connectioncallback_;
};

