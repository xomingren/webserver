#pragma once

#include <atomic>
#include <set>
#include <vector>

#include "channel_class.h"
#include "timestamp_class.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue
{
public:   
    explicit TimerQueue(EventLoop* loop);
    TimerQueue(const Channel&) = delete;
    TimerQueue& operator =(const TimerQueue&) = delete;
    ~TimerQueue();

    TimerId AddTimer(TimerCallback cb,
        Timestamp when,
        double interval);

    void Cancel(TimerId timerid);    
private:
    using Entry =  std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    using ActiveTimer = std::pair<Timer*, int64_t>;
    using ActiveTimerSet = std::set<ActiveTimer>;

    // FIXME: use unique_ptr<Timer> instead of raw pointers.
   // This requires heterogeneous comparison lookup (N3465) from C++14
   // so that we can find an T* in a set<unique_ptr<T>>.
  
    void AddTimerInLoop(Timer* timer);
    void CancelInLoop(TimerId timerid);
    // called when timerfd alarms
    void HandleRead();
    // move out all expired timers
    std::vector<Entry> GetExpired(Timestamp now);
    void Reset(const std::vector<Entry>& expired, Timestamp now);

    bool Insert(Timer* timer);

private:
    EventLoop* loop_;
    const int timerFD_;
    Channel timerfdchannel_;
    // Timer list sorted by expiration
    TimerList timers_;

    // for cancel()
    ActiveTimerSet activetimers_;
    bool callingexpiredtimers_; /* atomic */
    ActiveTimerSet cancelingtimers_;
};
