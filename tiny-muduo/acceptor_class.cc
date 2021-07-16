#include "acceptor_class.h"

#include <sys/socket.h> //for accept setsockopt
#include <arpa/inet.h>//for sockaddr_in
#include <errno.h> //for errno
#include <fcntl.h> // for fcntl()
#include <iostream>//for cout

using namespace std;

Acceptor* Acceptor::ptr_this = nullptr;

Acceptor::Acceptor(FD epollfd)
  : acceptchannel_(nullptr),
    epollfd_(epollfd),
    listenfd_(-1)
{
    ptr_this = this;
}

void Acceptor::OnAccept(FD socketfd)
{
    SocketFD connectfd;
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(sockaddr_in);
    connectfd = accept(ptr_this->listenfd_, reinterpret_cast<sockaddr*>(&cliaddr), &clilen);
    if (connectfd > 0)
    {
        cout << "new connection from "
            << "[" << inet_ntoa(cliaddr.sin_addr)
            << ":" << ntohs(cliaddr.sin_port) << "]"
            << " new socket fd:" << connectfd
            << endl;
    }
    else
    {
        cout << "accept error, connfd:" << connectfd
            << " errno:" << errno << endl;
    }
    fcntl(connectfd, F_SETFL, O_NONBLOCK); //no-block io
    
    ptr_this->callbackfunc_(connectfd);//callback in tcpserver,new tcpconnection and save in map there
}

void Acceptor::set_callbackfunc(CallBackFunc callbackfunc)
{
    callbackfunc_ = callbackfunc;
}

void Acceptor::Start()
{
    listenfd_ = CreateSocketAndListenOrDie();
    acceptchannel_ = new Channel(epollfd_, listenfd_); // Memory Leak !!!
    acceptchannel_->set_callbackfunc(OnAccept);
    acceptchannel_->EnableRead();
}

SocketFD Acceptor::CreateSocketAndListenOrDie()
{
    int optionval = 1;
    SocketFD listensocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    fcntl(listensocket, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(int));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if (-1 == bind(listensocket, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl;
    }

    if (-1 == listen(listensocket, kMaxListenFd))//2nd param is the queue size kernel keeped that done with 3 handshake and haven't been accepted
    {
        cout << "listen error, errno:" << errno << endl;
    }
    return listensocket;
}
