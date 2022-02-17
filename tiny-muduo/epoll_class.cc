#include "epoll_class.h"

#include <sys/epoll.h>
#include <errno.h> //for errno

#include <iostream>//for cout
#include <string.h>//for memset

using namespace std;

Epoll::Epoll()
{
    epollfd_ = ::epoll_create(1);
    if (epollfd_ <= 0)
        cout << "epoll_create error, errno:" << epollfd_ << endl;
}

void Epoll::Poll(std::vector<Channel*>* channels)
{
    epoll_event events[kMaxEvents];
    FD fds = epoll_wait(epollfd_, events, kMaxEvents, -1);
    if (fds == -1)
    {
        cout << "epoll_wait error, errno:" << errno << endl;
            return;
    }
    for (int i = 0; i < fds; ++i)
    {
        Channel* channel = static_cast<Channel*>(events[i].data.ptr);
        channel->set_revent(events[i].events);//mark this channel with what event happend
        channels->push_back(channel);
    }
}

void Epoll::Update(Channel* channel)
{
    if (channel->get_epollstatus() == EpollStatus::kNew)// a new one, add with EPOLL_CTL_ADD
    {
        epoll_event event;
        memset(&event, 0, sizeof event);
        event.data.ptr = channel;
        event.events = channel->get_event();
        channel->set_epollstatus(EpollStatus::kAdded);//when a new channel updated once, the second time we update it will fall into the 'else' down below
        epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->get_socketfd(), &event);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
    }
    else// update existing one with EPOLL_CTL_MOD/DEL
    {
        epoll_event event;
        event.data.ptr = channel;
        event.events = channel->get_event();//EPOLLOUT etc
        epoll_ctl(epollfd_, EPOLL_CTL_MOD, channel->get_socketfd(), &event);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
    }
    
}
