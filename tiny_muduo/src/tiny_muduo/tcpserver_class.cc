#include "tcpserver_class.h"

#include <stdio.h>

#include <memory>//for make_shared
#include <string>

#include "acceptor_class.h"
#include "commonfunction.h"
#include "eventloop_class.h"
#include "eventloopthreadpool_class.h"

#include "log.h"

using namespace std;

TcpServer::TcpServer(EventLoop* loop)
    : acceptor_(make_unique<Acceptor>(loop)),
      loop_(loop),
      nextconnid_(1),
      threadpool_(make_shared<EventLoopThreadPool>(loop))
{
    acceptor_->set_callbackfunc(bind(&TcpServer::OnNewConnection, this, placeholders::_1));
}

TcpServer::~TcpServer()
{
    loop_->AssertInLoopThread();
    LOG_INFO << "TcpServer::~TcpServer [" << "] destructing";

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
    EventLoop* ioloop = threadpool_->GetNextLoop(); 
    char buf[64];
    snprintf(buf, sizeof buf, "-%d", nextconnid_);
    ++nextconnid_;
    string connName = buf;
    TcpConnectionPtr connection = make_shared<TcpConnection>(ioloop, connName, socketfd);
    connection->Init();
    connections_[connName] = connection;
    connection->set_messagecallback(messagecallback_);
    connection->set_connectioncallback(connectioncallback_);
    connection->set_writecompletecallback(writecompletecallback_);
    connection->set_highwatermarkcallback(highwatermarkcallback_, highwatermark_);
    connection->set_closecallback(
        std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1)); // FIXME: unsafe
    ioloop->RunInLoop(std::bind(&TcpConnection::OnConnectEstablished, connection));
}

void TcpServer::SetThreadNum(int numthreads)
{
    assert(0 <= numthreads);
    threadpool_->SetThreadNum(numthreads);
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->AssertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop ["
        << "] - connection " << conn->get_name();
    size_t n = connections_.erase(conn->get_name());
    (void)n;
    assert(n == 1);
    EventLoop* ioloop = conn->get_loop();
    ioloop->QueueInLoop(
        std::bind(&TcpConnection::OnConnectDestroyed, conn));
}

void TcpServer::Start()
{
    //when new connection comes,acceptor use a connectfd to save it and make it unblock,
    //then give the connectfd back to here,because a newconnection(decode sompute encode) is the save level with the acceptor,not belong to it 
    //acceptor_->Start();
    int isstarted = 0;
    if (started_.compare_exchange_weak(isstarted,1))
    {
        threadpool_->Start(threadinitcallback_);

        assert(!acceptor_->Listenning());
        loop_->RunInLoop(std::bind(&Acceptor::Start, get_pointer(acceptor_)));
    }
}
