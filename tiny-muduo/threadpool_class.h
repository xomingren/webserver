#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "noncopyable_class.h"
#include "thread_class.h"
#include "thread_annotations.h"

class ThreadPool : noncopyable
{
public:
	typedef std::function<void()> Task;

	explicit ThreadPool(const std::string& nameArg = std::string("ThreadPool"));
	~ThreadPool();

	// Must be called before start().
	void set_maxqueuesize(int maxsize) 
	{ maxqueuesize_ = maxsize; }

	void set_threadinitcallback(const Task& cb)
	{  threadinitcallback_ = cb;  }

	void Start(int numThreads);
	void Stop();

	const std::string& Name() const
	{
		return name_;
	}

	size_t QueueSize() const;

	// Could block if maxQueueSize > 0
	void Run(Task f);

private:
	bool IsFull() const REQUIRES(mutex_);
	void RunInThread();
	Task Take();

	mutable std::mutex mutex_;
	std:: condition_variable notempty_ GUARDED_BY(mutex_);
	std::condition_variable notfull_ GUARDED_BY(mutex_);
	std::string name_;
	Task threadinitcallback_;
	std::vector<std::unique_ptr<Thread>> threads_;
	std::deque<Task> queue_ GUARDED_BY(mutex_);
	size_t maxqueuesize_;
	bool running_;
};