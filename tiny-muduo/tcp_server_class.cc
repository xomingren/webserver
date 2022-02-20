#include "tcp_server_class.h"

#include <iostream>//for cout

using namespace std;

TcpServer::TcpServer(EventLoop* loop)
    : acceptor_(nullptr),  
      loop_(loop)
{
}

void TcpServer::OnNewConnection(SocketFD socketfd)
{
    //acceptor get new connect callback to here,every tcpconnection had a channel* (to bind epollfd and socketfd)
    //class channel will choose different callback(onaccept or on event) according to the
    //owner(acceptor or tcpconnection)
    TcpConnection* connection = new TcpConnection(loop_, socketfd);//memory leak
    connections_[socketfd] = connection;
    connection->set_messagecallback(messagecallback_);
    connection->set_connectioncallback(connectioncallback_);
    connection->set_writecompletecallback(writecompletecallback_);
    connection->set_highwatermarkcallback(highwatermarkcallback_, highwatermark_);
    connection->OnConnectEstablished();
}

void TcpServer::Start()
{
    acceptor_ = new Acceptor(loop_);//memory leak
    //when new connection comes,acceptor use a connectfd to save it and make it unblock,
    //then give the connectfd back to here,because a newconnection(decode sompute encode) is the save level with the acceptor,not belong to it 
    acceptor_->set_callbackfunc(bind(&TcpServer::OnNewConnection,this,placeholders::_1));
    acceptor_->Start();
}
