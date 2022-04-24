#include "acceptor_class.h"

#include <arpa/inet.h>//for sockaddr_in
#include <errno.h> //for errno
#include <fcntl.h> // for fcntl()

#include "eventloop_class.h"

#include "log.h"

using namespace std;

Acceptor::Acceptor(EventLoop* loop)
  : loop_(loop),
    listenfd_(CreateSocketAndListenOrDie()),
    acceptchannel_(loop, listenfd_),
    listenning_(false)
{
    acceptchannel_.set_readcallback(bind(&Acceptor::OnAccept, this));
}

Acceptor::~Acceptor()
{
    acceptchannel_.DisableAll();
    acceptchannel_.Remove();
    ::close(listenfd_);
}

void Acceptor::OnAccept()
{
    SocketFD connectfd;
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(sockaddr_in);
    connectfd = accept(listenfd_, reinterpret_cast<sockaddr*>(&cliaddr), &clilen);
    if (connectfd > 0)
    {
        LOG_INFO << "new connection from "
            << "[" << inet_ntoa(cliaddr.sin_addr)
            << ":" << ntohs(cliaddr.sin_port) << "]"
            << " new socket fd:" << connectfd;
    }
    else
    {
        LOG_CRIT << "accept error, connfd:" << connectfd
            << " errno:" << errno;
    }

    // non-block
    int flags = ::fcntl(connectfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    ::fcntl(connectfd, F_SETFL, flags);
    // FIXME check

    // close-on-exec :when fork(),close all fd aotumaticlly
    flags = ::fcntl(connectfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ::fcntl(connectfd, F_SETFD, flags);
    
    callbackfunc_(connectfd);//callback in tcpserver,new tcpconnection and save in map there
}

void Acceptor::Start()
{
    loop_->AssertInLoopThread();
    listenning_ = true;
    //the event int acceptchannel_ marked as EPOLLIN | EPOLLET ,means this channel will be triggered when in event comes(epoll_waits return)
    //most important step because it'll sign its own channels' socketfd to epollfd
    acceptchannel_.EnableRead();
}

SocketFD Acceptor::CreateSocketAndListenOrDie()
{
    int optionval = 1;
    SocketFD listensocket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    sockaddr_in servaddr;
    fcntl(listensocket, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(int));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);//fixme, hard code

    if (-1 == bind(listensocket, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)))
    {
        LOG_CRIT << "bind error, errno:" << errno;
    }

    if (-1 == listen(listensocket, SOMAXCONN))//2nd param is the queue size kernel keeped that done with 3 handshake and haven't been accepted
    {//there are 2 kinds of queue in tcp handshake stage, 2nd param used in full-conn queue,half-conn queue'size <= SOMAXCONN
        LOG_CRIT << "listen error, errno:" << errno;
    }
    return listensocket;
}
