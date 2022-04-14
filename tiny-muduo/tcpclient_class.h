#pragma once

#include <mutex>
#include <string>

//#include "connector_class.h"
#include "noncopyable_class.h"
#include "tcpconnection_class.h"
#include "thread_annotations.h"

class Connector;
using ConnectorPtr = std::shared_ptr<Connector>;

class TcpClient : noncopyable
{
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const string& host, uint16_t port);
  explicit TcpClient(EventLoop* loop);
  ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

  void Connect();
  void Disconnect();
  void Stop();

  TcpConnectionPtr Connection() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  EventLoop* get_loop() const { return loop_; }
  bool Retry() const { return retry_; }
  void EnableRetry() { retry_ = true; }

  const std::string& get_name() const
  { return name_; }

  /// Set connection callback.
  /// Not thread safe.
  void set_connectioncallback(ConnectionCallback cb)
  { connectioncallback_ = std::move(cb); }

  /// Set message callback.
  /// Not thread safe.
  void set_messagecallback(MessageCallback cb)
  { messagecallback_ = std::move(cb); }

  /// Set write complete callback.
  /// Not thread safe.
  void set_writecompletecallback(WriteCompleteCallback cb)
  { writecompletecallback_ = std::move(cb); }

 private:
  /// Not thread safe, but in loop
  void NewConnection(int sockfd);
  /// Not thread safe, but in loop
  void RemoveConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_; // avoid revealing Connector
  const std::string name_;
  ConnectionCallback connectioncallback_;
  MessageCallback messagecallback_;
  WriteCompleteCallback writecompletecallback_;
  bool retry_;   // atomic
  bool connect_; // atomic
  // always in loop thread
  int nextconnid_;
  mutable std::mutex mutex_;
  TcpConnectionPtr connection_ GUARDED_BY(mutex_);
};

