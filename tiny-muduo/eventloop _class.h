#pragma once

#include "channel_class.h"
#include "epoll_class.h"

class EventLoop
{
public:
    EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator =(const EventLoop&) = delete;
    ~EventLoop() = default;

    void Loop();
    void Update(Channel* channel);
private:
    bool quit_;
    Epoll* poller_;
};

