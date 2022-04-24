#pragma once

#include <atomic>
#include <functional>
#include <latch>
#include <memory>
#include <thread>

#include "noncopyable_class.h"

class Thread : noncopyable
{
public:
	using ThreadFunc = std::function<void()>;

	Thread(ThreadFunc, const std::string& name = std::string());
	// FIXME: make it movable in C++11
	~Thread();

	void Start();
	void Join(); // return join()

	bool Started() const { return started_; }
	// pthread_t pthreadId() const { return pthreadId_; }
	pid_t Tid() const { return tid_; }
	const std::string& Name() const { return name_; }

	static int NumCreated() { return numcreated_.load(); }

private:
	void SetDefaultName();

	bool started_;
	bool joined_;
	std::thread  thread_;
	pid_t tid_;// used to  identity one thread
	ThreadFunc func_;
	std::string name_;
	std::latch latch_;

	static std:: atomic<int32_t> numcreated_; //how many thread been created
};

