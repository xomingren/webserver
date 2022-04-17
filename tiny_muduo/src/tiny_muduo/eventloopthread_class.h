#pragma once

#include <condition_variable>
#include <mutex>
#include <string>

#include "noncopyable_class.h"
#include "thread_class.h"
#include "thread_annotations.h"

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
    ~EventLoopThread();

    EventLoop* StartLoop();

private:
    void ThreadFunc();

    EventLoop* loop_ GUARDED_BY(mutex_);
    bool exiting_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_ GUARDED_BY(mutex_);
    ThreadInitCallback callback_;
};