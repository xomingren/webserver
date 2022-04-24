#include "timerqueue_class.h"

#include <assert.h>//for assert
#include <inttypes.h>//for UINTPTR_MAX
#include <sys/timerfd.h>//for timerfd
#include <string.h>//for memset
#include <unistd.h>//for read

#include "eventloop_class.h"
#include "timer_class.h"
#include "timerid_class.h"

#include "log.h"

using namespace std;

namespace detail
{

    int CreateTimerFD()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
            TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0)
        {
            LOG_CRIT << "Failed in timerfd_create()";
        }
        return timerfd;
    }

    struct timespec HowMuchTimeFromNow(Timestamp when)//how much microsec
    {
        int64_t microseconds = when.get_microsecondssinceepoch()
            - Timestamp::Now().get_microsecondssinceepoch();
        if (microseconds < 100)
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(
            microseconds / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(
            (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
        return ts;
    }

    void ReadTimerFD(int timerfd, Timestamp now)
    {
        uint64_t howmany;
        ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
        LOG_INFO << "TimerQueue::HandleRead() " << howmany << " at " << now.ToString(false)<<"\n";
        if (n != sizeof howmany)
        {
            LOG_CRIT << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
        }
    }

    void ResetTimerFD(int timerfd, Timestamp expiration)
    {
        // wake up loop by timerfd_settime()
        struct itimerspec newValue;
        struct itimerspec oldValue;
        memset(&newValue, 0, sizeof newValue);
        memset(&oldValue, 0, sizeof oldValue);
        newValue.it_value = HowMuchTimeFromNow(expiration);//how much microsec expiration - now
        int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
        if (ret)
        {
            LOG_INFO << "timerfd_settime()";
        }
    }

}  // namespace detail

using namespace std;
using namespace detail;

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
    timerFD_(CreateTimerFD()),
    timerfdchannel_(loop, timerFD_),
    timers_(),
    callingexpiredtimers_(false)
{
    timerfdchannel_.set_readcallback(
        std::bind(&TimerQueue::HandleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdchannel_.EnableRead();
}

TimerQueue::~TimerQueue()
{
    timerfdchannel_.DisableAll();
    timerfdchannel_.Remove();
    ::close(timerFD_);
    // do not remove channel, since we're in EventLoop::dtor();
    for (const Entry& timer : timers_)
    {
        delete timer.second;
    }
}

TimerId TimerQueue::AddTimer(TimerCallback cb,
    Timestamp when,
    double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval);
    loop_->RunInLoop(
        std::bind(&TimerQueue::AddTimerInLoop, this, timer));
    return TimerId(timer, timer->sequence());
}

void TimerQueue::Cancel(TimerId timerId)
{
    loop_->RunInLoop(
        std::bind(&TimerQueue::CancelInLoop, this, timerId));
}

void TimerQueue::AddTimerInLoop(Timer* timer)
{
    //loop_->assertInLoopThread();
    bool earliestchanged = Insert(timer);

    if (earliestchanged)
    {
        ResetTimerFD(timerFD_, timer->expiration()); //wake up loop by timerfd_settime()
    }
}

void TimerQueue::CancelInLoop(TimerId timerId)
{
    //loop_->assertInLoopThread();
    assert(timers_.size() == activetimers_.size());
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activetimers_.find(timer);
    if (it != activetimers_.end())
    {
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); 
        (void)n;
        delete it->first; // FIXME: no delete please
        activetimers_.erase(it);
    }
    else if (callingexpiredtimers_)//processing ,insert and deal with it next round
    {
        cancelingtimers_.insert(timer);
    }
    assert(timers_.size() == activetimers_.size());
}

void TimerQueue::HandleRead()//wake up from loop by timerfd_settime
{
    //loop_->assertInLoopThread();
    Timestamp now(Timestamp::Now());
    ReadTimerFD(timerFD_, now);//read 8 bytes from fd

    std::vector<Entry> expired = GetExpired(now);//calling expired timers

    callingexpiredtimers_ = true;
    cancelingtimers_.clear();
    // safe to callback outside critical section
    for (const Entry& it : expired)
    {
        it.second->run();
    }
    callingexpiredtimers_ = false;

    Reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::GetExpired(Timestamp now)
{
    assert(timers_.size() == activetimers_.size());
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));//problemmark when 2 timer had the same time,UINTPTR_MAX ensure that sentry is still the last(compare 1st then compare 2nd)
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    for (const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activetimers_.erase(timer);
        assert(n == 1); 
        (void)n;
    }

    assert(timers_.size() == activetimers_.size());
    return expired;
}

void TimerQueue::Reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextexpire;

    for (const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat()//if expired timer meant to repeat,and not cacelling,we need to restart it
            && cancelingtimers_.find(timer) == cancelingtimers_.end())
        {
            it.second->Restart(now);//restart from now,interval  have been set in timer in ctor
            Insert(it.second);//insert to timerslist and activelist same time
        }
        else//if expired timer meant to run once, abandon it
        {
            // FIXME move to a free listf
            delete it.second; // FIXME: no delete please
        }
    }

    if (!timers_.empty())
    {
        nextexpire = timers_.begin()->second->expiration();//next smallest expiration
    }

    if (nextexpire.Valid())
    {
        ResetTimerFD(timerFD_, nextexpire);//use timerfd_settime to call handleread when nextexpire comes
    }
}

bool TimerQueue::Insert(Timer* timer)
{
    //loop_->assertInLoopThread();
    assert(timers_.size() == activetimers_.size());
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }
    {
        std::pair<TimerList::iterator, bool> result
            = timers_.insert(Entry(when, timer));
        assert(result.second); (void)result;
    }
    {
        std::pair<ActiveTimerSet::iterator, bool> result
            = activetimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == activetimers_.size());
    return earliestChanged;
}
