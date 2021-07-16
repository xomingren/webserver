#include "channel_class.h"

#include <sys/epoll.h>

Channel::Channel(FD epollfd,FD socketfd_):
	epollfd_(epollfd), 
	socketfd_(socketfd_),
	event_(0),
	revent_(0)
{
}

void Channel::set_callbackfunc(CallBackFunc callbackfunc)
{
	callbackfunc_ = callbackfunc;
}

void Channel::set_revent(uint32_t revent)
{
	revent_ = revent;
}

void Channel::EnableRead()
{
	event_ =  EPOLLIN | EPOLLET ;
	update();
}

void Channel::HandleEvent()
{
	if(revent_ & EPOLLIN)
		callbackfunc_(socketfd_);
}

void Channel::update()
{
	epoll_event ev;
	ev.data.ptr = this;
	ev.events = event_;
	epoll_ctl(epollfd_, EPOLL_CTL_ADD, socketfd_, &ev);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
}
