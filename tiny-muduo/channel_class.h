#pragma once
#ifndef _CHANNEL_CLASS_H_
#define _CHANNEL_CLASS_H_
#include "define.h"

//#include "eventloop _class.h"

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
	void set_index(int index)
	{ index_ = index; }
	int get_index() const
	{ return index_; }
	void set_revent(uint32_t revent)//events happening now
	{ revent_ = revent; }
	uint32_t get_event() const
	{ return event_; }
	SocketFD get_socketfd() const
	{ return socketfd_; }
	void EnableRead();
	void EnableWrite();
	void DisableWrite();
	bool IsWriting();
	void HandleEvent();
private:
	void Update();//update my sockedfd to epfd

	FD socketfd_;
	int index_;
	uint32_t event_;
	uint32_t revent_;
	
	EventCallback readcallbackfunc_;
	EventCallback writecallbackfunc_;
	EventLoop* loop_;
};
#endif

