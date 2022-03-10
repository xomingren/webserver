#include "eventloop_class.h"

#include <sys/eventfd.h>
#include <unistd.h>//for read/write

#include <assert.h>
#include <iostream>
#include <vector>

#include "timerqueue_class.h"

using namespace std;

EventLoop::EventLoop()
  :  threadId_(CurrentThread::Tid()),//fixme,
     quit_(false),
	 poller_(new Epoll()),
     wakeupfd_(CreateEventFd()),
     wakeupchannel_(new Channel(this, wakeupfd_)),
     timerqueue_(new TimerQueue(this))
{
    wakeupchannel_->set_readcallback(bind(&EventLoop::HandleRead,this));//fixme
    wakeupchannel_->EnableRead();
}

EventLoop::~EventLoop()
{
    //wakeupchannel_->disableAll();
    //wakeupchannel_->remove();
    ::close(wakeupfd_);
    //t_loopInThisThread = NULL;
}

void EventLoop::Loop()
{
    while (!quit_)
    {
        cout << "epoll waiting等待..." << endl;
        vector<Channel*> channels;
        pollreturntime_ = poller_->Poll(&channels);

        for (const auto& it : channels)
        {
            //choose different callback
            //1.if channel is acceptchannel ,use OnAccept() and the socketfd returned to generate a new tcpconnection in tcpserver
            //2.if channel is tcpconnection, use OnEvent() to decode compute encode etc
            //  2.1  if epoll_out triggered,use HandleWrite()
            it->HandleEvent(pollreturntime_);
        }
        DoPendingFunctors();
    }
}

void EventLoop::Update(Channel* channel)
{
    poller_->Update(channel);
}
void EventLoop::RunInLoop(Functor cb)
{
    if (IsInLoopThread())
    {
        cb();
    }
    else
    {
        QueueInLoop(std::move(cb));
    }
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

TimerId EventLoop::RunAt(Timestamp time, TimerCallback cb)
{
    return timerqueue_->AddTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::RunAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::Now(), delay));
    return RunAt(time, std::move(cb));
}

TimerId EventLoop::RunEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::Now(), interval));
    return timerqueue_->AddTimer(std::move(cb), time, interval);
}

void EventLoop::Cancel(TimerId timerId)
{
    return timerqueue_->Cancel(timerId);
}

void EventLoop::AbortNotInLoopThread()
{
    cout << "EventLoop::abortNotInLoopThread - EventLoop " << this
        << " was created in threadId_ = " << threadId_
        << ", current thread id = " << CurrentThread::Tid();
}

bool EventLoop::HasChannel(Channel* channel)
{
    assert(channel->OwnerLoop() == this);
    AssertInLoopThread();
    //return poller_->hasChannel(channel); //fixme
    return true;
}

void EventLoop::RemoveChannel(Channel* channel)
{
    /*assert(channel->ownerLoop() == this);
    AssertInLoopThread();
    if (eventhandling_)
    {
        assert(currentActiveChannel_ == channel ||
            std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);*/
}