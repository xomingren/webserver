#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "noncopyable_class.h"

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
	using ThreadInitCallback = std::function<void(EventLoop*)>;

	EventLoopThreadPool(EventLoop* baseloop);
	~EventLoopThreadPool();

/// Set the number of threads for handling input.
///
/// Always accepts new connection in loop's thread.
/// Must be called before @c start
/// @param numThreads
/// - 0 means all I/O in loop's thread, no thread will created.
///   this is the default value.
/// - 1 means all I/O in another thread.
/// - N means a thread pool with N threads, new connections
///   are assigned on a round-robin basis.
	void SetThreadNum(int numthreads) { numthreads_ = numthreads; }
	void Start(const ThreadInitCallback& cb = ThreadInitCallback());

	// valid after calling start()
	/// round-robin
	EventLoop* GetNextLoop();

	/// with the same hash code, it will always return the same EventLoop
	EventLoop* GetLoopForHash(size_t hashCode);

	std::vector<EventLoop*> GetAllLoops();

	bool Started() const
	{
		return started_;
	}

private:

	EventLoop* baseloop_;
	bool started_;
	int numthreads_;
	int next_;
	std::vector<std::unique_ptr<EventLoopThread>> threads_;
	std::vector<EventLoop*> loops_;
};