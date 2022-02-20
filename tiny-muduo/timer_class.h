#pragma once

#include <atomic>

#include "define.h"
#include "timestamp_class.h"

class Timer
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval)
        : callback_(std::move(cb)),//consider std::function as std::string¡£ pass by value and std::move
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        sequence_(++s_numcreated_)
    { }

    void run() const
    {
        callback_();
    }

    Timestamp expiration() const { return expiration_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void Restart(Timestamp now);

    static int64_t get_numcreated() { return s_numcreated_.load(); }

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic<int64_t> s_numcreated_;
};