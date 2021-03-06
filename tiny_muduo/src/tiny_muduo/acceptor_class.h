#pragma once

#include "channel_class.h"
#include "define.h"
#include "noncopyable_class.h"

class Acceptor :public noncopyable
{
public:
	using NewConnectionCallback = std::function<void(FD sockfd)>;

	Acceptor(EventLoop* loop);
	~Acceptor();

	void set_callbackfunc(NewConnectionCallback cb)
	{ callbackfunc_ = cb; }

	bool Listenning() const { return listenning_; }

	void Start();

private:
	void OnAccept();
	SocketFD CreateSocketAndListenOrDie();

	EventLoop* loop_;
	FD listenfd_;
	Channel acceptchannel_;
	NewConnectionCallback callbackfunc_;	
	bool listenning_;
};

