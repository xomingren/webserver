#include "thread_class.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>

#include <exception>
#include <type_traits>

#include "currentthread_class.h"
#include "timestamp_class.h"



namespace detail
{

    unsigned int Gettid()
    {
        return static_cast<unsigned int>(::syscall(SYS_gettid));
    }

    void AfterFork()
    {
        CurrentThread::t_cachedtid = 0;
        CurrentThread::t_threadname = "main";
        CurrentThread::Tid();
        // no need to call pthread_atfork(nullptr, nullptr, &afterFork);
    }

    class ThreadNameInitializer
    {
    public:
        ThreadNameInitializer()
        {
            CurrentThread::t_threadname = "main";
            CurrentThread::Tid();
            pthread_atfork(nullptr, nullptr, &AfterFork);
        }
    };

    ThreadNameInitializer init;

    struct ThreadData
    {
        using ThreadFunc = Thread::ThreadFunc;
        ThreadFunc func_;
        std::string name_;
        pid_t* tid_;
        std::latch* latch_;

        ThreadData(ThreadFunc func,
            const std::string& name,
            pid_t* tid,
            std::latch* latch)
            : func_(std::move(func)),
                name_(name),
                tid_(tid),
                latch_(latch)
        { }

        void RunInThread()
        {
            *tid_ = CurrentThread::Tid();
            tid_ = nullptr;
            latch_->count_down();
            latch_ = nullptr;

            CurrentThread::t_threadname = name_.empty() ? "tinymuduothread" : name_.c_str();
            ::prctl(PR_SET_NAME, CurrentThread::t_threadname);
            try
            {
                func_();
                CurrentThread::t_threadname = "finished";
            }
            /* catch (const Exception& ex)//fixme
            {
                CurrentThread::t_threadName = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", ex.what());
                fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
                abort();
            }*/
            catch (const std::exception& ex)
            {
                CurrentThread::t_threadname = "crashed";
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", ex.what());
                abort();
            }
            catch (...)
            {
                CurrentThread::t_threadname = "crashed";
                fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                throw; // rethrow
            }
        }
    };

    void* StartThread(void* obj)
    {
        ThreadData* data = static_cast<ThreadData*>(obj);
        data->RunInThread();
        delete data;
        return nullptr;
    }

}  // namespace detail

    void CurrentThread::CacheTid()
    {
        if (t_cachedtid == 0)
        {
            t_cachedtid = detail::Gettid();
            t_tidstringlength = snprintf(t_tidstring, sizeof t_tidstring, "%5d ", t_cachedtid);
        }
    }

    bool CurrentThread::IsMainThread()
    {
        return Tid() == ::getpid();
    }

    void CurrentThread::SleepMicroSeconds(int64_t microsec)
    {
        struct timespec ts = { 0, 0 };
        ts.tv_sec = static_cast<time_t>(microsec / Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(microsec % Timestamp::kMicroSecondsPerSecond * 1000);
        ::nanosleep(&ts, nullptr);
    }

    std::atomic<int32_t> Thread::numcreated_;

    Thread::Thread(ThreadFunc func, const std::string& n)
        : started_(false),
          joined_(false),
          tid_(0),
          func_(std::move(func)),
          name_(n),
          latch_(1)
    {
        SetDefaultName();
    }

    Thread::~Thread()
    {
        if (started_ && !joined_)
        {
            thread_.detach();
        }
    }

    void Thread::SetDefaultName()
    {
        int num = ++Thread:: numcreated_;
        if (name_.empty())
        {
            char buf[32];
            snprintf(buf, sizeof buf, "Thread%d", num);
            name_ = buf;
        }
    }

    void Thread::Start()//will block
    {
        assert(!started_);
        started_ = true;
        // FIXME: move(func_)
        detail::ThreadData* data = new detail::ThreadData(func_, name_, &tid_, &latch_);
        thread_ = std::thread(std::bind(&detail::StartThread, static_cast<void*>(data)));//this thread continue runs and block at wait, StartThread() begin runs in another thread
        latch_.wait();//block wait till count down to zero(another threads RunInThread -> latch_.count_down(1)
        assert(tid_ > 0);
    }

    void Thread::Join()
    {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        thread_.join();
    }
