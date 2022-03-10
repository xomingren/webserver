#include <latch>
#include <unistd.h>  // usleep
#include <stdio.h>

#include <iostream>

#include "current_thread_class.h"
#include "threadpool_class.h"

using namespace std;

void print()
{
    printf("tid=%d\n", CurrentThread::Tid());
}

void printString(const std::string& str)
{
    printf("%s\n", str.c_str());
    usleep(100 * 1000);
}

void test(int maxSize)
{
    printf("Test ThreadPool with max queue size = %d\n", maxSize);
    ThreadPool pool("MainThreadPool");
    pool.set_maxqueuesize(maxSize);
    pool.Start(5);

    printf("Adding\n");
    pool.Run(print);
    pool.Run(print);
    for (int i = 0; i < 100; ++i)
    {
        char buf[32];
        snprintf(buf, sizeof buf, "task %d", i);
        pool.Run(std::bind(printString, std::string(buf)));
    }
    printf("Done\n");

    std::latch lat(1);
    pool.Run(std::bind(&std::latch::count_down, &lat,1));
    lat.wait();
    pool.Stop();
}

//int main(int args, char** argv)
//{
//    test(0);
//    test(1);
//    test(5);
//    test(10);
//    test(50);
//
//    return 0;
//}