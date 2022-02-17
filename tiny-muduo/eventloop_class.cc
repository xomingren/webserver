#include "eventloop_class.h"

#include <iostream>
#include <vector>

#include <sys/eventfd.h>
#include <unistd.h>

using namespace std;

EventLoop::EventLoop()
	:quit_(false),
	 poller_(new Epoll())
{
    wakeupfd_ = CreateEventFd();
    wakeupchannel_ = new Channel(this, wakeupfd_); // Memory Leak !!!
    wakeupchannel_->set_readcallbackfunc(bind(&EventLoop::HandleRead,this));//fixme
    wakeupchannel_->EnableRead();
}

void EventLoop::Loop()
{
    while (!quit_)
    {
        cout << "epoll waiting等待..." << endl;
        vector<Channel*> channels;
        poller_->Poll(&channels);

        for (const auto& it : channels)
        {
            //choose different callback
            //1.if channel is acceptchannel ,use OnAccept() and the socketfd returned to generate a new tcpconnection in tcpserver
            //2.if channel is tcpconnection, use OnEvent() to decode compute encode etc
            //  2.1  if epoll_out triggered,use HandleWrite()
            it->HandleEvent();
        }
        DoPendingFunctors();
    }
}

void EventLoop::Update(Channel* channel)
{
    poller_->Update(channel);
}
void EventLoop::QueueInLoop(Functor cb)
{
    pendingfunctors_.push_back(std::move(cb));
    Wakeup();
}

void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n = ::write(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        cout << "EventLoop::Wakeup() writes " << n << " bytes instead of 8" << endl;
    }
}

int EventLoop::CreateEventFd()
{
    FD evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        cout << "Failed in eventfd" << endl;
    }
    return evtfd;
}

void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        cout << "EventEventLoop::handleRead() reads " << n << " bytes instead of 8" << endl;
    }
}

void EventLoop::HandleWrite()
{

}

void EventLoop::DoPendingFunctors()
{
    vector<Functor>tmp;
    tmp.swap(pendingfunctors_);
    for (const auto& functor : tmp)
    {
        functor();
    }
}