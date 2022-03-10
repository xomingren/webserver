#pragma once

#include <memory>

#include "define.h"
#include "noncopyable_class.h"

//this class relate combine socketfd and its callbackfunction
//use update to relate socketfd and epollfd
class Channel : noncopyable
{
public:
	Channel(EventLoop* loop,FD socketfd);
	~Channel();

	void set_readcallback(ReadEventCallback cb)
	{ readcallback_ = std::move(cb); }

	void set_writecallback(EventCallback cb)
	{ writecallback_ = std::move(cb); }

	void set_closecallback(EventCallback cb)
	{ closecallback_ = std::move(cb); }
	//void set_writewritecompletefunc(Functor cb)
	//{ writecompletecallbackfunc_ = std::move(cb); }

	void set_epollstatus(EpollStatus index)
	{ epollstatus_ = index; }

	EpollStatus get_epollstatus() const
	{ return epollstatus_; }

	void set_revent(uint32_t revent)//events happening now
	{ revents_ = revent; }

	uint32_t get_event() const
	{ return events_; }

	SocketFD get_FD() const
	{ return FD_; }

	bool IsNoneEvent() const
	{ return events_ == kNoneEvent; }

	void EnableRead();
	void EnableWrite();
	void DisableWrite();
	void DisableAll() 
	{  events_ = kNoneEvent; Update();  }
	bool IsWriting();
	void HandleEvent(Timestamp receivetime);
	
	void Tie(const std::shared_ptr<void>& obj);
	EventLoop* OwnerLoop() 
	{ return loop_; }
	void Remove();

private:
	void Update();//update my sockedfd to epfd
	void HandleEventWithGuard(Timestamp receivetime);

	EpollStatus epollstatus_;

	static const int kNoneEvent;
	static const int kReadEvent;
	static const int kWriteEvent;

	EventLoop* loop_;
	const FD FD_;//sockfd,timefd
	int events_;
	int revents_;
	
	
	std::weak_ptr<void> tie_;
	bool tied_;
	bool eventhandling_;
	bool addedtoloop_;
	ReadEventCallback readcallback_;
	EventCallback writecallback_;
	EventCallback closecallback_;
	//Functor writecompletecallbackfunc_;	
};


