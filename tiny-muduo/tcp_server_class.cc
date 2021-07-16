#include "tcp_server_class.h"

#include <iostream>//for cout

#include<vector>

using namespace std;

TcpServer* TcpServer::ptr_this = nullptr;

TcpServer::TcpServer()
    :epollfd_(-1),
    acceptor_(nullptr)
{
    ptr_this = this;
}

void TcpServer::OnNewConnection(SocketFD socketfd)
{
    //acceptor get new connect callback to here,every tcpconnection had a channel* (to bind epollfd and socketfd)
    //class channel will choose different callback(onaccept or on event) according to the
    //owner(acceptor or tcpconnection)
    TcpConnection* connection = new TcpConnection(ptr_this->epollfd_, socketfd);//meory leak
    ptr_this->connections_[socketfd] = connection;
}

void TcpServer::Start()
{
    epollfd_ = epoll_create(1);
    if (epollfd_ <= 0)
        cout << "epoll_create error, errno:" << epollfd_ << endl;
    acceptor_ = new Acceptor(epollfd_);//memory leak
    acceptor_->set_callbackfunc(OnNewConnection);
    acceptor_->Start();

    for (;;)
    {
        vector<Channel*> channels;
        channels.reserve(kMaxEvents);
        cout << "epoll waiting..." << endl;
        FD fds = epoll_wait(epollfd_, events_, kMaxEvents, -1);
        if (fds == -1)
        {
            cout << "epoll_wait error, errno:" << errno << endl;
            break;
        }
        for (int i = 0; i < fds; ++i)
        {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->set_revent(events_[i].events);//sign epollin|epollet event
            channels.push_back(channel);
        }

        for (const auto& it : channels)
        {
            it->HandleEvent();//choose different callback
        }
    }
}
