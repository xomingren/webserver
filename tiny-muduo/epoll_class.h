#pragma once

#include <vector>

//#include "channel_class.h"
#include "define.h"
#include "noncopyable_class.h"
//#include "timestamp_class.h"

//a epollfd manage class,
//once epoll_wait return,
//get the active fd(and the pointer to channel within it) from events[]
//build a vector<channel*>, return to eventloop.eventloop will traverse the vector,trigger its callbackfunction

class Channel;

class Epoll :public noncopyable
{
public:
	Epoll();
	~Epoll();

	Timestamp Poll(std::vector<Channel*>* channels);
	void Update(Channel* channel);


private:
	static const uint16_t kMaxEvents = 500;
	FD epollfd_;
};

