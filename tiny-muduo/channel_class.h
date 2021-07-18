#pragma once
#ifndef _CHANNEL_CLASS_H_
#define _CHANNEL_CLASS_H_
#include "define.h"

class EventLoop;
//this class relate combine socketfd and its callbackfunction
//use update to relate socketfd and epollfd
class Channel
{
public:
	Channel(EventLoop* loop,FD socketfd);
	Channel(const Channel&) = delete;
	Channel& operator =(const Channel&) = delete;
	~Channel() = default;

	void set_callbackfunc(SocketCallBack);
	void set_revent(uint32_t);//events happening now
	uint32_t get_event() const;
	SocketFD get_socketfd() const;
	void EnableRead();
	void HandleEvent();
private:
	void Update();//update my sockedfd to epfd

	FD socketfd_;
	uint32_t event_;
	uint32_t revent_;
	
	SocketCallBack callbackfunc_;
	EventLoop* loop_;
};
#endif

