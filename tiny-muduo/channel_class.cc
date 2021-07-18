#include "channel_class.h"

#include <sys/epoll.h>

#include "eventloop _class.h"

Channel::Channel(EventLoop* loop,FD socketfd_)
	: socketfd_(socketfd_),
	  event_(0),
	  revent_(0),
	  loop_(loop)
{
}

void Channel::set_callbackfunc(SocketCallBack callbackfunc)
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
	Update();
}

void Channel::HandleEvent()
{
	if(revent_ & EPOLLIN)
		callbackfunc_(socketfd_);
}

void Channel::Update()
{
	//channel dont directly related to pollfd,but through eventloop 
	loop_->Update(this);
}
uint32_t Channel::get_event() const
{
	return event_;
}

SocketFD Channel::get_socketfd() const
{
	return socketfd_;
}
