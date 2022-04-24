#include "eventloop_class.h"

#include <sys/eventfd.h>
#include <unistd.h>//for read/write

#include <assert.h>
#include <vector>

#include "epoll_class.h"
#include "timerqueue_class.h"

#include "log.h"

using namespace std;

EventLoop::EventLoop()
  :  threadId_(CurrentThread::Tid()),//fixme,
     quit_(false),
     eventhandling_(false),
	 poller_(make_unique<Epoll>()),
     wakeupfd_(CreateEventFd()),
     wakeupchannel_(make_unique<Channel>(this, wakeupfd_)),
     timerqueue_(make_unique<TimerQueue>(this))
{
    wakeupchannel_->set_readcallback(bind(&EventLoop::HandleRead,this));//fixme
    wakeupchannel_->EnableRead();
}

EventLoop::~EventLoop()
{
    wakeupchannel_->DisableAll();
    wakeupchannel_->Remove();
    ::close(wakeupfd_);
    //t_loopInThisThread = nullptr;
}

void EventLoop::Loop()
{
    while (!quit_)
    {
        vector<Channel*> channels;
        pollreturntime_ = poller_->Poll(&channels);

        eventhandling_ = true;
        for (const auto& it : channels)
        {
            //choose different callback
            //1.if channel is acceptchannel ,use OnAccept() and the socketfd returned to generate a new tcpconnection in tcpserver
            //2.if channel is tcpconnection, use OnEvent() to decode compute encode etc
            //  2.1  if epoll_out triggered,use HandleWrite()
            it->HandleEvent(pollreturntime_);
        }
        eventhandling_ = false;
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
        LOG_CRIT << "writes " << n << " bytes instead of 8";
    }
}

int EventLoop::CreateEventFd()
{
    FD evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_CRIT << "failed in eventfd()";
    }
    return evtfd;
}

void EventLoop::HandleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupfd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_INFO << "reads " << n << " bytes instead of 8";
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
    LOG_CRIT << "EventLoop::AbortNotInLoopThread - EventLoop " << reinterpret_cast<char*>(this)
        << " was created in threadId_ = " << threadId_
        << ", current thread id = " << CurrentThread::Tid();
}

bool EventLoop::HasChannel(Channel* channel)
{
    assert(channel->OwnerLoop() == this);
    AssertInLoopThread();
    //return poller_->hasChannel(channel); //fixme
    return false;
}

void EventLoop::RemoveChannel(Channel* channel)
{
    assert(channel->OwnerLoop() == this);
    AssertInLoopThread();
    if (eventhandling_)
    {
        //assert(currentActiveChannel_ == channel || std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    //poller_->RemoveChannel(channel);
}

void EventLoop::Quit()//problemmark how and when quit
{
    quit_ = true;
    // There is a chance that loop() just executes while(!quit_) and exits,
    // then EventLoop destructs, then we are accessing an invalid object.
    // Can be fixed using mutex_ in both places.
    if (!IsInLoopThread())
    {
        Wakeup();
    }
}