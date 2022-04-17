#include "eventloopthread_class.h"

#include <assert.h>

#include "eventloop_class.h"

using namespace std;

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::ThreadFunc, this), "1"),
      mutex_(),
      callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr) // not 100% race-free, eg. threadFunc could be running callback_.
    {
        // still a tiny chance to call destructed object, if threadFunc exits just now.
        // but when EventLoopThread destructs, usually programming is exiting anyway.
        loop_->Quit();
        thread_.Join();
    }
}

EventLoop* EventLoopThread::StartLoop()
{
    assert(!thread_.Started());
    thread_.Start();// ThreadFunc() runs

    EventLoop* loop = nullptr;
    {
        unique_lock<mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock); //wakeup by notify_one in ThreadFunc()
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;

    if (callback_)
    {
        callback_(&loop);
    }

    {
        unique_lock<mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.Loop();
    //assert(exiting_);
    unique_lock<mutex> lock(mutex_);
    loop_ = nullptr;
}