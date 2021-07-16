#pragma once

#include <vector>

#include "define.h"
#include "channel_class.h"
//a epollfd manage class,
//once epoll_wait return,
//get the active fd(and the pointer to channel within it) from events[]
//build a vector<channel*>, return to eventloop.eventloop will traverse the vector,trigger its callbackfunction
class Epoll
{
public:
	Epoll();
	Epoll(const Epoll&) = delete;
	Epoll& operator =(const Epoll&) = delete;
	~Epoll() = default;

	void Poll(std::vector<Channel*>* channels);
	void Update(Channel* channel);
private:
	FD epollfd_;
};

