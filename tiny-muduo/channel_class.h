#pragma once

#include "define.h"

//this class relate combine socketfd and its callbackfunction
//use update to relate socketfd and epollfd
class Channel
{
public:
	Channel(EventLoop* loop,FD socketfd);
	Channel(const Channel&) = delete;
	Channel& operator =(const Channel&) = delete;
	~Channel() = default;

	void set_readcallbackfunc(EventCallback cb)
	{ readcallbackfunc_ = std::move(cb); }

	void set_writecallbackfunc(EventCallback cb)
	{ writecallbackfunc_ = std::move(cb); }

	//void set_writewritecompletefunc(Functor cb)
	//{ writecompletecallbackfunc_ = std::move(cb); }

	void set_epollstatus(EpollStatus index)
	{ epollstatus_ = index; }

	EpollStatus get_epollstatus() const
	{ return epollstatus_; }

	void set_revent(uint32_t revent)//events happening now
	{ revent_ = revent; }

	uint32_t get_event() const
	{ return event_; }

	SocketFD get_FD() const
	{ return FD_; }

	void EnableRead();
	void EnableWrite();
	void DisableWrite();
	bool IsWriting();
	void HandleEvent();

private:
	void Update();//update my sockedfd to epfd

	FD FD_;//sockfd,timefd
	EpollStatus epollstatus_;
	uint32_t event_;
	uint32_t revent_;
	
	EventCallback readcallbackfunc_;
	EventCallback writecallbackfunc_;
	//Functor writecompletecallbackfunc_;
	EventLoop* loop_;
};


