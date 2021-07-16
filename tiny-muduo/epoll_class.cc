#include "epoll_class.h"

#include <sys/epoll.h>
#include <errno.h> //for errno
#include <iostream>//for cout

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
    epoll_event ev;
    ev.data.ptr = channel;
    ev.events = channel->get_event();
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->get_socketfd(), &ev);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
}
