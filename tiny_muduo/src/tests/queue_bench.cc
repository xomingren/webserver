#include <stdio.h>
#include <unistd.h>

#include <functional>
#include <latch>
#include <map>
#include <memory>
#include <string>
#include <string.h>//for strlen
#include <vector>

#include "../tiny_muduo/currentthread_class.h"
#include "../tiny_muduo/queue_class.h"
#include "../tiny_muduo/thread_class.h"
#include "../tiny_muduo/timestamp_class.h"

using namespace std;

class Bench
{
public:
    Bench(int numThreads)
        : latch_(numThreads)
        //threads_(numThreads)
    {
        for (int i = 0; i < numThreads; ++i)
        {
            char name[32];
            snprintf(name, sizeof name, "work thread %d", i);
            threads_.emplace_back(std::make_unique<Thread>(std::bind(&Bench::threadFunc, this),string(name,strlen(name))));
        }
        for (auto& thr : threads_)
        {
            thr->Start();
        }
    }

    void run(int times)
    {
        printf("waiting for count down latch\n");
        latch_.wait();//threadnum->0
        printf("all threads started\n");
        for (int i = 0; i < times; ++i)
        {
           Timestamp now(Timestamp::Now());
            queue_.put(now);
            usleep(1000);
        }
    }

    void joinAll()
    {
        for (size_t i = 0; i < threads_.size(); ++i)
        {
            queue_.put(Timestamp::Invalid());
        }

        for (auto& thr : threads_)
        {
            thr->Join();
        }
    }

private:

    void threadFunc()
    {
        printf("tid=%d, %s started\n",
            CurrentThread::Tid(),
            CurrentThread::Name());

        using del = int;
        using count = int;
        map<del, count> delays;
        latch_.count_down();
        bool running = true;
        while (running)
        {
            Timestamp t(queue_.take());
            Timestamp now(Timestamp::Now());
            if (t.Valid())
            {
                int delay = static_cast<int>(timeDifference(now, t) * 1000000);
                // printf("tid=%d, latency = %d us\n",
                //        muduo::CurrentThread::tid(), delay);
                ++delays[delay];
            }
            running = t.Valid();
        }

        printf("tid=%d, %s stopped\n",
            CurrentThread::Tid(),
            CurrentThread::Name());
        for (const auto& delay : delays)
        {
            printf("tid = %d, delay = %d, count = %d\n",
                CurrentThread::Tid(),
                delay.first, delay.second);
        }
    }

    BlockingQueue<Timestamp> queue_;
    latch latch_;
    vector<std::unique_ptr<Thread>> threads_;
};

int main(int argc, char* argv[])
{
     int threads = 10;

    Bench t(threads);//thread->start() in ctor,and block at it
    t.run(10000);
    t.joinAll();
}