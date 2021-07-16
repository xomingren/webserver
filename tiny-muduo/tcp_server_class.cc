#include "tcp_server_class.h"

#include <iostream>//for cout

using namespace std;

TcpServer* TcpServer::ptr_this = nullptr;

TcpServer::TcpServer(EventLoop* loop)
    : acceptor_(nullptr),  
      loop_(loop)
{
    ptr_this = this;
}

void TcpServer::OnNewConnection(SocketFD socketfd)
{
    //acceptor get new connect callback to here,every tcpconnection had a channel* (to bind epollfd and socketfd)
    //class channel will choose different callback(onaccept or on event) according to the
    //owner(acceptor or tcpconnection)
    TcpConnection* connection = new TcpConnection(ptr_this->loop_, socketfd);//memory leak
    ptr_this->connections_[socketfd] = connection;
}

void TcpServer::Start()
{
    acceptor_ = new Acceptor(loop_);//memory leak
    //when new connection comes,acceptor use a connectfd to save it and make it unblock,
    //then give the connectfd back to here,because a newconnection(decode sompute encode) is the save level with the acceptor,not belong to it 
    acceptor_->set_callbackfunc(OnNewConnection);
    acceptor_->Start();
}
