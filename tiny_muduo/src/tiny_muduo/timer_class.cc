#include "timer_class.h"

std::atomic<int64_t> Timer::s_numcreated_;

void Timer::Restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::Invalid();
    }
}