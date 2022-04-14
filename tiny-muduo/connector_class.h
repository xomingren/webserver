#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include "noncopyable_class.h"

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop);
  ~Connector();

  void set_newconnectioncallback(const NewConnectionCallback& cb)
  { newconnectioncallback_ = cb; }

  void Start();  // can be called in any thread
  void Restart();  // must be called in loop thread
  void Stop();  // can be called in any thread

 private:
  enum class States : char
  { 
      kDisconnected, kConnecting, kConnected
  };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void set_state(States s) { state_ = s; }
  void StartInLoop();
  void StopInLoop();
  void Connect();
  void Connecting(int sockfd);
  void HandleWrite();
  void HandleError();
  void Retry(int sockfd);
  int RemoveAndResetChannel();
  void ResetChannel();

  EventLoop* loop_;
  bool connect_; // atomic
  std:: atomic<States> state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback newconnectioncallback_;
  int retrydelayms_;
};
