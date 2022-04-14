#include "eventloopthreadpool_class.h"

#include <assert.h>//for assert
#include <stdio.h>//for snprintf

#include "commonfunction.h"
#include "eventloop_class.h"
#include "eventloopthread_class.h"

using namespace std;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop)
    : baseloop_(baseloop),
      started_(false),
      numthreads_(0),
      next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::Start(const ThreadInitCallback& cb)
{
    assert(!started_);
    baseloop_->AssertInLoopThread();

    started_ = true;

    for (int i = 0; i < numthreads_; ++i)
    {
        //EventLoopThread* t = new EventLoopThread(cb);
        //threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        threads_.emplace_back(make_unique<EventLoopThread>(cb));
        loops_.push_back(threads_.back()->StartLoop());
    }
    if (numthreads_ == 0 && cb)
    {
        cb(baseloop_);
    }
}

EventLoop* EventLoopThreadPool::GetNextLoop()
{
    baseloop_->AssertInLoopThread();
    assert(started_);
    EventLoop* loop = baseloop_;

    if (!loops_.empty())
    {
        // round-robin
        loop = loops_[next_];
        ++next_;
        if (implicit_cast<size_t>(next_) >= loops_.size())
        {
            next_ = 0;
        }
    }
    return loop;
}

EventLoop* EventLoopThreadPool::GetLoopForHash(size_t hashCode)
{
    baseloop_->AssertInLoopThread();
    EventLoop* loop = baseloop_;

    if (!loops_.empty())
    {
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops()
{
    baseloop_->AssertInLoopThread();
    assert(started_);
    if (loops_.empty())
    {
        return std::vector<EventLoop*>(1, baseloop_);
    }
    else
    {
        return loops_;
    }
}