#pragma once

#include "define.h"
#include "channel_class.h"
#include "eventloop_class.h"

class Acceptor
{
public:
	Acceptor(EventLoop* loop);
	Acceptor(const Acceptor&) = delete;
	Acceptor& operator =(const Acceptor&) = delete;
	~Acceptor() = default;

	void OnAccept();
	void set_callbackfunc(NewConnectionCallback cb)
	{ callbackfunc_ = cb; }
	void Start();
private:
	SocketFD CreateSocketAndListenOrDie();

	Channel* acceptchannel_;
	NewConnectionCallback callbackfunc_;
	EventLoop* loop_;
	
	FD listenfd_;	
};

