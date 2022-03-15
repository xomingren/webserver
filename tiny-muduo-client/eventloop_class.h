#pragma once

#include <memory>//for unique_ptr
#include <vector>

#include "currentthread_class.h"
#include "define.h"
#include "epoll_class.h"
#include "timerid_class.h"
#include "timestamp_class.h"

class TimerQueue;

class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator =(const EventLoop&) = delete;
    ~EventLoop(); // force out-line dtor, for std::unique_ptr members. problemmark

    void Loop();
    void Update(Channel* channel);

    void AssertInLoopThread()
    {
        if (!IsInLoopThread())
        {
            AbortNotInLoopThread();
        }
    }

    bool IsInLoopThread() const
    {  return threadId_ == CurrentThread::Tid();  }

    void AbortNotInLoopThread();

    /// Runs callback immediately in the loop thread.
 /// It wakes up the loop, and run the cb.
 /// If in the same loop thread, cb is run within the function.
 /// Safe to call from other threads.
    void RunInLoop(Functor cb);
    void QueueInLoop(Functor cb);

    void HandleRead();
    void HandleWrite();

    bool HasChannel(Channel* channel);
    void RemoveChannel(Channel* channel);
    void Quit();

    // timers

///
/// Runs callback at 'time'.
/// Safe to call from other threads.
///
    TimerId RunAt(Timestamp time, TimerCallback cb);
    ///
    /// Runs callback after @c delay seconds.
    /// Safe to call from other threads.
    ///
    TimerId RunAfter(double delay, TimerCallback cb);
    ///
    /// Runs callback every @c interval seconds.
    /// Safe to call from other threads.
    ///
    TimerId RunEvery(double interval, TimerCallback cb);
    ///
    /// Cancels the timer.
    /// Safe to call from other threads.
    ///
    void Cancel(TimerId timerId);
private:
    void Wakeup();
    int CreateEventFd();
    void DoPendingFunctors();

    const pid_t threadId_;
    bool quit_;
    bool eventhandling_; /* atomic */
    std::unique_ptr <Epoll> poller_;
    Timestamp pollreturntime_;
    FD wakeupfd_;
    std::unique_ptr<Channel> wakeupchannel_;
    std::vector<Functor> pendingfunctors_;
    std::unique_ptr<TimerQueue> timerqueue_;
};

