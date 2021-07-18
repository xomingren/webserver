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

	void OnAccept(FD socketfd);
	void set_callbackfunc(SocketCallBack);
	void Start();
private:
	SocketFD CreateSocketAndListenOrDie();

	Channel* acceptchannel_;
	SocketCallBack callbackfunc_;
	EventLoop* loop_;
	
	FD listenfd_;	
};

