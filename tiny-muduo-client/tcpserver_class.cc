#include "tcpserver_class.h"

#include <stdio.h>

#include <iostream>//for cout
#include <memory>//for make_shared
#include <string>

using namespace std;

TcpServer::TcpServer(EventLoop* loop)
    : acceptor_(nullptr),  
      loop_(loop),
      nextconnid_(1)
{
}

TcpServer::~TcpServer()
{
    loop_->AssertInLoopThread();
    cout << "TcpServer::~TcpServer [" << "] destructing";

    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();//--counter
        conn->get_loop()->RunInLoop(
            std::bind(&TcpConnection::OnConnectDestroyed, conn));
    }
}

void TcpServer::OnNewConnection(SocketFD socketfd)
{
    //acceptor get new connect callback to here,every tcpconnection had a channel* (to bind epollfd and socketfd)
    //class channel will choose different callback(onaccept or on event) according to the
    //owner(acceptor or tcpconnection)
    loop_->AssertInLoopThread();
    //EventLoop* ioLoop = threadPool_->getNextLoop(); //fixeme
    char buf[64];
    snprintf(buf, sizeof buf, "-%d", nextconnid_);
    ++nextconnid_;
    string connName = buf;
    TcpConnectionPtr connection = make_shared<TcpConnection>( loop_, connName, socketfd);
    connection->Init();
    connections_[connName] = connection;
    connection->set_messagecallback(messagecallback_);
    connection->set_connectioncallback(connectioncallback_);
    connection->set_writecompletecallback(writecompletecallback_);
    connection->set_highwatermarkcallback(highwatermarkcallback_, highwatermark_);
    connection->set_closecallback(
        std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1)); // FIXME: unsafe
    loop_->RunInLoop(std::bind(&TcpConnection::OnConnectEstablished, connection));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->AssertInLoopThread();
    cout << "TcpServer::removeConnectionInLoop ["
        << "] - connection " << conn->get_name();
    size_t n = connections_.erase(conn->get_name());
    (void)n;
    assert(n == 1);
    EventLoop* ioLoop = conn->get_loop();
    ioLoop->QueueInLoop(
        std::bind(&TcpConnection::OnConnectDestroyed, conn));
}

void TcpServer::Start()
{
    acceptor_ = new Acceptor(loop_);//memory leak
    //when new connection comes,acceptor use a connectfd to save it and make it unblock,
    //then give the connectfd back to here,because a newconnection(decode sompute encode) is the save level with the acceptor,not belong to it 
    acceptor_->set_callbackfunc(bind(&TcpServer::OnNewConnection,this,placeholders::_1));
    acceptor_->Start();
}
