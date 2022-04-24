#include "threadpool_class.h"

#include <assert.h>

using namespace std;

ThreadPool::ThreadPool(const string& namearg)
  : mutex_(),
    notempty_(),
    notfull_(),
    name_(namearg),
    maxqueuesize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
    if (running_)
    {
        Stop();
    }
}

void ThreadPool::Start(int numthreads)
{
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numthreads);
    for (int i = 0; i < numthreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i + 1);
        threads_.emplace_back(std::make_unique<Thread>(std::bind(&ThreadPool::RunInThread, this), name_ + id));
        threads_[i]->Start();
    }
    if (numthreads == 0 && threadinitcallback_)
    {
        threadinitcallback_();
    }
}

void ThreadPool::Stop()
{
    {
        lock_guard<mutex> lock(mutex_);
        running_ = false;
        notempty_.notify_all();
    }
    for (auto& thr : threads_)
    {
        thr->Join();
    }
}

size_t ThreadPool::QueueSize() const
{
    lock_guard<mutex> lock(mutex_);
    return queue_.size();
}

void ThreadPool::Run(Task task)
{
    if (threads_.empty())
    {
        task();
    }
    else
    {
        unique_lock<mutex> lock(mutex_);
        while (IsFull())
        {
            notfull_.wait(lock);
        }
        assert(!IsFull());

        queue_.push_back(std::move(task));
        notempty_. notify_one();
    }
}

ThreadPool::Task ThreadPool::Take()
{
    unique_lock<mutex> lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty() && running_)
    {
        notempty_.wait(lock);
    }
    Task task;
    if (!queue_.empty())
    {
        task = queue_.front();
        queue_.pop_front();
        if (maxqueuesize_ > 0)
        {
            notfull_.notify_one();
        }
    }
    return task;
}

bool ThreadPool::IsFull() const
{
    //mutex_.assertLocked();//FIXME
    return maxqueuesize_ > 0 && queue_.size() >= maxqueuesize_;
}

void ThreadPool::RunInThread()
{
    try
    {
        if (threadinitcallback_)
        {
            threadinitcallback_();
        }
        while (running_)
        {
            Task task(Take());
            if (task)
            {
                task();
            }
        }
    }
    /*catch (const Exception& ex)//fix me
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }*/
    catch (const std::exception& ex)
    {
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...)
    {
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}