#include "channel_class.h"

#include <sys/epoll.h>

#include "eventloop_class.h"

Channel::Channel(EventLoop* loop,FD socketfd)
	: socketfd_(socketfd),
	  epollstatus_(EpollStatus::kNew),//mark itself as a new channel
	  event_(0),
	  revent_(0),
	  loop_(loop)
{
}

void Channel::EnableRead()
{
	event_ =  EPOLLIN | EPOLLET ;
	Update();
}

void Channel::EnableWrite()
{
	event_ |= EPOLLOUT;
	Update();
}

void Channel::DisableWrite()
{
	event_ &= ~EPOLLOUT;
	Update();
}
// EPOLLOUT means Writing now will not block.

bool Channel::IsWriting()  
{
	return event_ & EPOLLOUT;
}

void Channel::HandleEvent()
{
	if(revent_ & EPOLLIN)
		readcallbackfunc_();
	if (revent_ & EPOLLOUT)
		writecallbackfunc_();
}

void Channel::Update()
{
	//channel dont directly related to pollfd,but through eventloop 
	loop_->Update(this);
}
