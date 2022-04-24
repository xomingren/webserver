#include "epoll_class.h"

#include <errno.h> //for errno
#include <sys/epoll.h>

#include <string.h>//for memset

#include "channel_class.h"

#include "log.h"

using namespace std;

Epoll::Epoll()
  : epollfd_(::epoll_create1(EPOLL_CLOEXEC))
{
    if (epollfd_ < 0)
        LOG_CRIT << "epoll_create1 error, errno:" << errno;
}

Epoll::~Epoll()
{
    ::close(epollfd_);
}

Timestamp Epoll::Poll(std::vector<Channel*>* channels)
{
    epoll_event events[kMaxEvents];//events happend will fiiling it
    FD fds = epoll_wait(epollfd_, events, kMaxEvents, -1);
    Timestamp now(Timestamp::Now());
    if (fds == -1)
    {
        LOG_CRIT << "epoll_wait error, errno:" << errno;
    }
    for (int i = 0; i < fds; ++i)
    {
        Channel* channel = static_cast<Channel*>(events[i].data.ptr);
        channel->set_revent(events[i].events);//mark this channel with what event happend
        channels->push_back(channel);
    }
    return now;
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
        epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->get_FD(), &event);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
    }
    else// update existing one with EPOLL_CTL_MOD/DEL
    {
        epoll_event event;
        event.data.ptr = channel;
        event.events = channel->get_event();//EPOLLOUT etc
        epoll_ctl(epollfd_, EPOLL_CTL_MOD, channel->get_FD(), &event);//3rd param means the fd to be added and concerned,4th param means what event we want the kernel concerned aboout the fd
    }
    
}
