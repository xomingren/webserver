#pragma once
#include "define.h"
#include "channel_class.h"
class Acceptor
{
public:
	Acceptor(FD epollfd);
	~Acceptor() = default;

	static void OnAccept(FD socketfd);
	void set_callbackfunc(CallBackFunc);
	void Start();
private:
	SocketFD CreateSocketAndListenOrDie();

	static Acceptor* ptr_this;
	Channel* acceptchannel_;
	CallBackFunc callbackfunc_;
	
	FD epollfd_;
	FD listenfd_;	
};

