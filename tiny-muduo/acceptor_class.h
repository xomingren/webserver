#pragma once

#include "define.h"
#include "channel_class.h"
#include "eventloop _class.h"

class Acceptor
{
public:
	Acceptor(EventLoop* loop);
	Acceptor(const Acceptor&) = delete;
	Acceptor& operator =(const Acceptor&) = delete;
	~Acceptor() = default;

	static void OnAccept(FD socketfd);
	void set_callbackfunc(CallBackFunc);
	void Start();
private:
	SocketFD CreateSocketAndListenOrDie();

	static Acceptor* ptr_this;
	Channel* acceptchannel_;
	CallBackFunc callbackfunc_;
	EventLoop* loop_;
	
	FD listenfd_;	
};

