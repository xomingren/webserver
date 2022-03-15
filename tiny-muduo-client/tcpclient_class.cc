#include "tcpclient_class.h"

#include "eventloop_class.h"

#include <stdio.h>  // snprintf

using namespace std;
using namespace placeholders;


namespace detail
{

void RemoveConnection(EventLoop* loop, const TcpConnectionPtr& conn)
{
  loop->QueueInLoop(std::bind(&TcpConnection::OnConnectDestroyed, conn));
}

void RemoveConnector(const ConnectorPtr& connector)
{
  //connector->
}

}  // namespace detail


TcpClient::TcpClient(EventLoop* loop)
  : loop_(loop),
    connector_(new Connector(loop)),
    connectioncallback_(DefaultConnectionCallback),
    messagecallback_(DefaultMessageCallback),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, placeholders::_1));
  // FIXME setConnectFailedCallback
  //LOG_INFO << "TcpClient::TcpClient[" << name_
  //         << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
  /*LOG_INFO << "TcpClient::~TcpClient[" << name_
           << "] - connector " << get_pointer(connector_);*/
  TcpConnectionPtr conn;
  bool unique = false;
  {
    lock_guard<mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn)
  {
    assert(loop_ == conn->get_loop());
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&detail::RemoveConnection, loop_, placeholders::_1);
    loop_->RunInLoop(
        std::bind(&TcpConnection::set_closecallback, conn, cb));
    if (unique)
    {
      conn->ForceClose();
    }
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->RunAfter(1, std::bind(&detail::RemoveConnector, connector_));
  }
}

void TcpClient::connect()
{
  // FIXME: check state
 /* LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to "
           << connector_->serverAddress().toIpPort();*/
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;

  {
    lock_guard<mutex> lock(mutex_);
    if (connection_)
    {
      connection_->Shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->AssertInLoopThread();

    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    char buf[32];
    snprintf(buf, sizeof buf, ":#%d", nextConnId_);
    ++nextConnId_;
    string connname = name_ + buf;
    TcpConnectionPtr conn(make_shared<TcpConnection>(loop_, connname, sockfd));
    conn->Init();
    conn->set_connectioncallback(connectioncallback_);
    conn->set_messagecallback(messagecallback_);
    conn->set_writecompletecallback(writeCompleteCallback_);
    conn->set_closecallback(
        std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
    {
        lock_guard<mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->OnConnectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->AssertInLoopThread();
  assert(loop_ == conn->get_loop());

  {
    lock_guard<mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->QueueInLoop(std::bind(&TcpConnection::OnConnectDestroyed, conn));
  if (retry_ && connect_)
  {
    connector_->restart();
  }
}

