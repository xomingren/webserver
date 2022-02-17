#pragma once

#include "buffer_class.h"
#include "channel_class.h"
#include "define.h"

#include <memory>

class TcpConnection :public std::enable_shared_from_this<TcpConnection>
{
public:
	TcpConnection(EventLoop* loop, FD socketfd);
	TcpConnection(const TcpConnection&) = delete;
	TcpConnection& operator =(const TcpConnection&) = delete;
	~TcpConnection() = default;

	void OnRecieve();
	void OnWrite();
	void OnConnectEstablished();

	void Send(Buffer* message);  // this one will swap data
	void Send(const std::string& message);
	void Send(const void* message, size_t len);

	void set_messagecallback(MessageCallBack messagecallback)
	{ messagecallback_ = messagecallback; }
	void set_connectioncallback(ConnectionCallBack connectioncallback)
	{ connectioncallback_ = connectioncallback; }
	void set_writecompletecallback(WriteCompleteCallback cb)
	{ writecompletecallback_ = std::move(cb); }
	void set_highwatermarkcallback(HighWaterMarkCallback cb, size_t highWaterMark)
	{ highwatermarkcallback_ = cb; highwatermark_ = highWaterMark; }
private:
	FD socketfd_;
	EventLoop* loop_;
	Channel* channel_;
	size_t highwatermark_;
	Buffer inbuf_;
	Buffer outbuf_;// FIXME: use list<Buffer> as output buffer.
	MessageCallBack messagecallback_;
	ConnectionCallBack connectioncallback_;
	WriteCompleteCallback writecompletecallback_;//called when outbuf_ is empty
	HighWaterMarkCallback highwatermarkcallback_;//called when wrote too many
};

