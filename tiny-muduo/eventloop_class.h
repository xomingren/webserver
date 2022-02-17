#pragma once

#include "define.h"
#include "epoll_class.h"

#include <vector>


class EventLoop
{
public:
    EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator =(const EventLoop&) = delete;
    ~EventLoop() = default;

    void Loop();
    void Update(Channel* channel);
    void QueueInLoop(Functor cb);

    void HandleRead();
    void HandleWrite();
private:
    void Wakeup();
    int CreateEventFd();
    void DoPendingFunctors();

    bool quit_;
    Epoll* poller_;
    FD wakeupfd_;
    Channel* wakeupchannel_;
    std::vector<Functor> pendingfunctors_;
};

