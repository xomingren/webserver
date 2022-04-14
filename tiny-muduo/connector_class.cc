#include "connector_class.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h> // for fcntl()
#include <memory.h>//for memset
#include <sys/socket.h>

#include "channel_class.h"
#include "commonfunction.h"
#include "eventloop_class.h"

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop)
  : loop_(loop),
    connect_(false),
    state_(States::kDisconnected),
    retrydelayms_(kInitRetryDelayMs)
{
  
}

Connector::~Connector()
{
  assert(!channel_);
}

void Connector::Start()
{
  connect_ = true;
  loop_->RunInLoop(std::bind(&Connector::StartInLoop, this)); // FIXME: unsafe
}

void Connector::StartInLoop()
{
  loop_->AssertInLoopThread();
  assert(state_ == States::kDisconnected);
  if (connect_)
  {
    Connect();
  }
  else
  {
   
  }
}

void Connector::Stop()
{
  connect_ = false;
  loop_->QueueInLoop(std::bind(&Connector::StopInLoop, this)); // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::StopInLoop()
{
  loop_->AssertInLoopThread();
  if (state_ == States::kConnecting)
  {
    set_state(States::kDisconnected);
    int sockfd = RemoveAndResetChannel();
    Retry(sockfd);
  }
}

void Connector::Connect()
{
  //int connsockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);

  //sockaddr_in serveraddr;
  //memset(&serveraddr, 0, sizeof(serveraddr));
  //fcntl(connsockfd, F_SETFL, O_NONBLOCK); //no-block io
  //int optionval = 1;
  //setsockopt(connsockfd, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(int));
  //serveraddr.sin_family = AF_INET;
  //serveraddr.sin_addr.s_addr = inet_addr("192.168.10.129");//fixme
  //serveraddr.sin_port = htons(11111);//fixme, hard code

  //int ret = ::connect(connsockfd,
  //    static_cast<const struct sockaddr*>(implicit_cast<const void*>(&serveraddr)),
  //    static_cast<socklen_t>(sizeof(struct sockaddr_in)));
  int connsockfd = ::socket(AF_INET, SOCK_STREAM /*| SOCK_NONBLOCK*/ | SOCK_CLOEXEC, IPPROTO_TCP);

  sockaddr_in serveraddr;
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr("192.168.10.129");//fixme
  serveraddr.sin_port = htons(11111);//fixme

  int ret = ::connect(connsockfd, static_cast<const struct sockaddr*>(implicit_cast<const void*>(&serveraddr)), static_cast<socklen_t>(sizeof(struct sockaddr_in)));
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno)
  {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      Connecting(connsockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      Retry(connsockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      ::close(connsockfd);
      break;

    default:
      ::close(connsockfd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::Restart()
{
  loop_->AssertInLoopThread();
  set_state(States::kDisconnected);
  retrydelayms_ = kInitRetryDelayMs;
  connect_ = true;
  StartInLoop();
}

void Connector::Connecting(int sockfd)
{
    set_state(States::kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->set_writecallback(
      std::bind(&Connector::HandleWrite, this)); // FIXME: unsafe

  // channel_->tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->EnableWrite();
}

int Connector::RemoveAndResetChannel()
{
  channel_->DisableAll();
  channel_->Remove();
  int sockfd = channel_->get_FD();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->QueueInLoop(std::bind(&Connector::ResetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::ResetChannel()
{
  channel_.reset();
}

void Connector::HandleWrite()
{
  if (state_ == States::kConnecting)
  {
    int sockfd = RemoveAndResetChannel();
    int err;
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);

    if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    } 
    if (err)
    {
      Retry(sockfd);
    }
   /* else if (sockets::isSelfConnect(sockfd))
    {
      retry(sockfd);
    }*/
    else
    {
      set_state(States::kConnected);
      if (connect_)
      {
        newconnectioncallback_(sockfd);
      }
      else
      {
        ::close(sockfd);
      }
    }
  }
  else
  {
    // what happened?
    assert(state_ == States::kDisconnected);
  }
}

void Connector::HandleError()
{

  if (state_ == States::kConnecting)
  {
    int sockfd = RemoveAndResetChannel();
    Retry(sockfd);
  }
}

void Connector::Retry(int sockfd)
{
  ::close(sockfd);
  set_state(States::kDisconnected);
  if (connect_)
  {
    loop_->RunAfter(retrydelayms_/1000.0,
                    std::bind(&Connector::StartInLoop, shared_from_this()));
    retrydelayms_ = std::min(retrydelayms_ * 2, kMaxRetryDelayMs);
  }
  else
  {
  }
}

