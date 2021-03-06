#pragma once

//#define BLOCKQUEUE

#ifdef BLOCKQUEUE

#include <assert.h>
#include <condition_variable>
#include <deque>
#include <mutex>

#include "noncopyable_class.h"
#include "thread_annotations.h"

template<typename T>
class BlockingQueue : noncopyable
{
public:
    BlockingQueue()
        : mutex_(),
        notempty_(),
        queue_()
    {
    }

    void put(const T& x)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(x);
        notempty_.notify_one(); // wait morphing saves us
    }

    void put(T&& x)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push_back(std::move(x));
        notempty_.notify_one();
    }

    T take()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty())
        {
            notempty_.wait(lock);
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;//mutable:can a;ways change even in const member funcion
    std::condition_variable notempty_ GUARDED_BY(mutex_);
    std::deque<T> queue_ GUARDED_BY(mutex_);
};
//problemmark
//GUARDED_BY : __attribute__((guarded_by(mutex_))
//guarded_by meant to keep (x) thread-safe with mutex_
//if thread want to use (x) ,we have to use mutex properly to keep it safe
//or complier will warning us in static complier period
//https://blog.csdn.net/weixin_42157432/article/details/115939656

#else

#include <atomic>
#include "noncopyable_class.h"

template<class T>
class BlockingQueue : noncopyable
{
    static const int ERROR_VOID_LIST = -9999;

    struct node
    {
        T value_;
        std::atomic<node*> next_;
        node(const T& x)
            : value_(x)
            , next_(nullptr)
        {}
    };

public:
    BlockingQueue()
    {
        tail_ = head_ = new node(T());
    }
    ~BlockingQueue()
    {
        node* cur = head_.load();
        while (cur)
        {
            node* next_ = cur->next_;
            delete cur;
            cur = next_;
        }
    }

    void put(const T& x)
    {
        node* newnode = new node(x);
        node* oldtail = nullptr;
        node* nullnode = nullptr;
        do
        {
            oldtail = tail_.load();
            nullnode = nullptr;//??????????nullnode????????oldtail->next_??????????
        } while (oldtail->next_.compare_exchange_weak(nullnode, newnode) != true);//????????????tail????????????????????????????????????????oldtail->next????????????????????????????????????tail????????
        //tail_.compare_exchange_weak(oldtail, newnode);
        tail_.store(newnode);
    }

    T take()
    {
        node* oldhead = nullptr;
        node* newhead = nullptr;
        T headvalue;
        again : do
        {
            oldhead = head_.load();
            newhead = oldhead->next_;
            if (newhead == nullptr)
                goto again;//return T();//fixme
            headvalue = newhead->value_;
        } while (head_.compare_exchange_weak(oldhead, newhead) != true);
        //delete oldhead; oldhead = nullptr;       
        return headvalue;
    }

    bool empty()
    {
        return head_.load() == tail_.load();
    }
private:
   
  
    std::atomic<node*> head_;//??????????????????????????????????????
    std::atomic<node*> tail_;
};

#endif