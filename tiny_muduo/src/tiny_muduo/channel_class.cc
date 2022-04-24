#include "channel_class.h"

#include <sys/epoll.h>

#include <assert.h>
#include <sstream>

#include "eventloop_class.h"

#include "log.h"

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
	events_ =  EPOLLIN /*| EPOLLET*/ ;//trigger control
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
	LOG_INFO << ReventsToString();
	if ((revents_ & EPOLLHUP/*close write and close*/) && !(revents_ & EPOLLIN))
	{
		LOG_WARN << "fd = " << FD_ << " : EPOLLHUP";
		
		if (closecallback_)
			closecallback_();
	}

	if (revents_ & EBADF)
	{
	LOG_WARN << "fd = " << FD_ << " : EBADF";
	}
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

string Channel::ReventsToString() const
{
	return EventsToString(FD_, revents_);
}

string Channel::EventsToString() const
{
	return EventsToString(FD_, events_);
}

string Channel::EventsToString(int fd, int ev)
{
	std::ostringstream oss;
	oss << "fd = " << fd << ": ";
	if (ev & EPOLLIN)
		oss << "IN ";
	if (ev & EPOLLPRI)//There is urgent data available for read(2) operations
		oss << "PRI ";
	if (ev & EPOLLOUT)
		oss << "OUT ";
	if (ev & EPOLLHUP)//response with RST. nonsense request 1.connect from not listening port 2.closed connect resend
		oss << "HUP ";
	if (ev & EPOLLRDHUP)// close connect 1. remote shutdown or close 2. local shutdown
		oss << "RDHUP ";
	if (ev & EPOLLERR)//local error
		oss << "ERR ";
	if (ev & EBADF)// fd invalid
		oss << "NVAL ";

	return oss.str();
}
