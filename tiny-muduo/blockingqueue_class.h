#pragma once

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
        notempty_(mutex_),
        queue_()
    {
    }

    void put(const T& x)
    {
        std::lock_guard lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify(); // wait morphing saves us
    }

    void put(T&& x)
    {
        std::lock_guard lock(mutex_);
        queue_.push_back(std::move(x));
        notempty_.notify();
    }

    T take()
    {
        std::lock_guard lock(mutex_);
        // always use a while-loop, due to spurious wakeup
        while (queue_.empty())
        {
            notempty_.wait();
        }
        assert(!queue_.empty());
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    size_t size() const
    {
        std::lock_guard lock(mutex_);
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

template<class T>
class BlockingQueue : noncopyable
{
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
            nullnode = nullptr;//交换失败时nullnode被设置为oldtail->next_，或者未定
        } while (oldtail->next_.compare_exchange_weak(nullnode, newnode) != true);//插入成功后，tail此时位倒数第二个节点，其他线程来读会发现oldtail->next永远不为空，会被卡住，直到本线程设置tail后移一位
        //tail_.compare_exchange_weak(oldtail, newnode);
        tail_.store(newnode);
    }

    T take()
    {
        //assert(!Empty());
        node* oldhead = nullptr;
        node* newhead = nullptr;
        T headvalue;
        do
        {
            oldhead = head_.load();
            newhead = oldhead->next_;
            if (newhead == nullptr)
                return ERROR_VOID_LIST;
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
    static const int ERROR_VOID_LIST = -9999;
  
    std::atomic<node*> head_;//实际上是一个哑节点，真正的头在其后一个
    std::atomic<node*> tail_;
};

#endif;