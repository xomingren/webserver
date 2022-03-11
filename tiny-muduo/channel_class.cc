#include "channel_class.h"

#include <sys/epoll.h>

#include <assert.h>
#include <iostream>

#include "eventloop_class.h"

using namespace std;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop,FD socketfd)
	: loop_(loop),
	  FD_(socketfd),
	  epollstatus_(EpollStatus::kNew),//mark itself as a new channel
	  events_(0),
	  revents_(0),
	  tied_(false),
	  eventhandling_(false),
	  addedtoloop_(false)

{
}

Channel::~Channel()
{
	assert(!eventhandling_);
	//assert(!addedtoloop_);
	if (loop_->IsInLoopThread())
	{
		assert(!loop_->HasChannel(this));
	}
}

void Channel::EnableRead()
{
	events_ =  EPOLLIN | EPOLLET ;
	Update();
}

void Channel::EnableWrite()
{
	events_ |= EPOLLOUT;
	Update();
}

void Channel::DisableWrite()
{
	events_ &= ~EPOLLOUT;
	Update();
}
// EPOLLOUT means Writing now will not block.

bool Channel::IsWriting()  
{
	return events_ & EPOLLOUT;
}

void Channel::HandleEvent(Timestamp receivetime)
{
	std::shared_ptr<void> guard;
	if (tied_)
	{
		guard = tie_.lock();
		if (guard)
		{
			HandleEventWithGuard(receivetime);
		}
	}
	else
	{
		HandleEventWithGuard(receivetime);
	}
}

void Channel::Tie(const std::shared_ptr<void>& obj)
{
	tie_ = obj;
	tied_ = true;
}

void Channel::HandleEventWithGuard(Timestamp receivetime)
{
	eventhandling_ = true;
	//LOG_TRACE << reventsToString();
	if ((revents_ & EPOLLHUP/*close write and close*/) && !(revents_ & EPOLLIN))
	{
		//if (logHup_)
		{
			cout << "fd = " << FD_ << " Channel::handle_event() EPOLLHUP";
		}
		if (closecallback_)
			closecallback_();
	}

	//if (revents_ & (EPOLLERR | EBADF))
	//{
	//	if (errorCallback_) errorCallback_();//fixme
	//}
	if (revents_ & (EPOLLIN | EPOLLPRI/*urgent come*/ | EPOLLRDHUP/*client close connection*/))
	{
		if (readcallback_) 
			readcallback_(receivetime);
	}
	if (revents_ & EPOLLOUT)
	{
		if (writecallback_) 
			writecallback_();
	}
	eventhandling_ = false;
}

void Channel::Update()
{
	//channel dont directly related to pollfd,but through eventloop 
	addedtoloop_ = true;
	loop_->Update(this);
}

void Channel::Remove()
{
	assert(IsNoneEvent());
	addedtoloop_ = false;
	loop_->RemoveChannel(this);
}
