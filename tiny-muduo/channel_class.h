#pragma once
#include <sys/epoll.h>

#include "define.h"
class Channel
{
public:
	Channel(FD,FD);
	~Channel() = default;
	void set_callbackfunc(CallBackFunc);
	void set_revent(uint32_t);//events happening
	void EnableRead();
	void HandleEvent();
private:
	void update();//sign connect sockedfd to epfd
	FD epollfd_;
	FD socketfd_;
	uint32_t revent_;
	uint32_t event_;
	CallBackFunc callbackfunc_;
};

