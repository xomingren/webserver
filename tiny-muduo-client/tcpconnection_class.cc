#include "tcpconnection_class.h"

#include <errno.h>//for errno
#include <unistd.h>//for read write close
#include <sys/socket.h>

#include <iostream>//for cout

#include "commonfunction.h"
#include "eventloop_class.h"

using namespace std;

void DefaultConnectionCallback(const TcpConnectionPtr& conn)
{
    cout << (conn->Connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void DefaultMessageCallback(const TcpConnectionPtr&,
    Buffer* buf,
    Timestamp)
{
    buf->RetrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, FD socketfd)
    : state_(StateE::kConnecting),
      loop_(loop),
      name_(nameArg),
      //socketfd_(socketfd),
      channel_(make_unique<Channel>(loop_, socketfd)),
      highwatermark_(64 * 1024 * 1024)//default value, but finally decide by lib user
{
    //problemmark call shared_from_this in ctor,will cause bad_weak_ptr error//https://www.cnblogs.com/lidabo/p/4058084.html
}

TcpConnection::~TcpConnection()
{
    cout << "TcpConnection::dtor[" << "] at " << this
        << " fd=" << channel_->get_FD();
    assert(state_ == StateE::kDisconnected);
}

void TcpConnection::HandleRead(Timestamp receivetime)
{
    loop_->AssertInLoopThread();
    const ssize_t readlength = inbuf_.ReadFd(channel_->get_FD());
    if (readlength > 0)
    {
        messagecallback_(shared_from_this(), &inbuf_, receivetime);
    }
    else if(0 == readlength)
    {
        HandleClose();
    }
}

void TcpConnection::HandleClose()
{
    loop_->AssertInLoopThread();
    cout << "fd = " << channel_->get_FD() << " state = " << StateToString();
    assert(state_ == StateE::kConnected || state_ == StateE::kDisconnecting);//fixme with cas
    // we don't close fd, leave it to dtor, so we can find leaks easily.
    set_state(StateE::kDisconnected);
    channel_->DisableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectioncallback_(guardThis);
    // must be the last line
    closecallback_(guardThis);
}

void TcpConnection::HandleWrite()
{
    loop_->AssertInLoopThread();
    if (channel_->IsWriting())
    {
        ssize_t wrotelength = ::write(channel_->get_FD(), outbuf_.Peek(), outbuf_.ReadableBytes());
        if (wrotelength > 0)
        {
            cout << "write " << wrotelength << " bytes data again" << endl;
            outbuf_.Retrieve(wrotelength);
            if (0 == outbuf_.ReadableBytes())//wrote done
            {
                channel_->DisableWrite();//remove EPOLLOUT
                if(writecompletecallback_)
                    loop_->QueueInLoop(bind(writecompletecallback_,shared_from_this())); //invoke onWriteComplete
            }
            if (state_ == StateE::kDisconnecting)
            {
                ShutdownInLoop();
            }
            //if wrote undone,this func will be triggered again
        }
    }
}

void TcpConnection::Send(const void* message, size_t len)
{
    Send(string_view(static_cast<const char*>(message), len));
}

void TcpConnection::Send(const string_view& message)
{
    if (state_ == StateE::kConnected)
    {
        if (loop_->IsInLoopThread())
        {
            SendInLoop(message);
        }
        else
        {
            void (TcpConnection::*fp)(const string_view& message) = &TcpConnection::SendInLoop;
            loop_->RunInLoop(
                std::bind(fp,
                    this,     // FIXME
                    string(message)));
            //std::forward<string>(message)));
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::Send(Buffer* buf)
{
    if (state_ == StateE::kConnected)
    {
        if (loop_->IsInLoopThread())
        {
            SendInLoop(buf->Peek(), buf->ReadableBytes());
            buf->RetrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const string_view & message) = &TcpConnection::SendInLoop;
            loop_->RunInLoop(
                std::bind(fp,
                    this,     // FIXME
                    buf->RetrieveAllAsString()));
            //std::forward<string>(message)));
        }
    }
}

void TcpConnection::SendInLoop(const string_view& message)
{
    SendInLoop(message.data(), message.size());
}

void TcpConnection::SendInLoop(const void* data, size_t len)
{
    loop_->AssertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == StateE::kDisconnected)
    {
        cout << "disconnected, give up writing";
        return;
    }
    // 1.if no thing in output queue, try writing directly
    if (!channel_->IsWriting() && outbuf_.ReadableBytes() == 0)
    {
        nwrote = ::write(channel_->get_FD(), data, len);
        if (nwrote >= 0)//wrote succeed,not sure how many bytes wrote
        {
            remaining = len - nwrote;
            if (remaining == 0 && writecompletecallback_)//all bytes wrote,call complete
            {
                loop_->QueueInLoop(std::bind(writecompletecallback_, shared_from_this()));
            }
        }
        else // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                cout << "TcpConnection::SendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true;
                }
            }
        }
    }

    //2.output queue isn't empty(means kernel cache cann't take more) 
    //3.or some bytes left unwrote,also means 2
    //for 2 and 3,append the remaining on outbufs' tail
    assert(remaining <= len);
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outbuf_.ReadableBytes();
        if (oldLen + remaining >= highwatermark_
            && oldLen < highwatermark_//ensure called in rising edge
            && highwatermarkcallback_)
        {
            loop_->QueueInLoop(std::bind(highwatermarkcallback_, shared_from_this(), oldLen + remaining));
        }
        outbuf_.Append(static_cast<const char*>(data) + nwrote, remaining);
        if (!channel_->IsWriting())//add EPOLLOUT
        {
            channel_->EnableWrite();
        }
        //when kernel has more space,it'll notice epollfd with epoll_wait
        //then trigger handlewrite()
    }
}

void TcpConnection::Shutdown()
{
    StateE nowstate = StateE::kConnected;
    do
    {
        nowstate = StateE::kConnected;

    } while (state_.compare_exchange_weak(nowstate, StateE::kDisconnecting) != true);
        // FIXME: shared_from_this()?
    loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
}

void TcpConnection::ForceClose()
{
    // FIXME: use compare and swap
    if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
    {
        set_state(StateE::kDisconnecting);
        loop_->QueueInLoop(std::bind(&TcpConnection::ForceCloseInLoop, shared_from_this()));
    }
}
void TcpConnection::ForceCloseInLoop()
{
    loop_->AssertInLoopThread();
    if (state_ == StateE::kConnected || state_ == StateE::kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        HandleClose();
    }
}

void TcpConnection::OnConnectEstablished()
{
    loop_->AssertInLoopThread();
    assert(state_ == StateE::kConnecting);
    set_state(StateE::kConnected);
    channel_->Tie(shared_from_this());
    channel_->EnableRead();

    connectioncallback_(shared_from_this());
}

void TcpConnection::OnConnectDestroyed()
{
    loop_->AssertInLoopThread();
    if (state_ == StateE::kConnected)
    {
        set_state(StateE::kDisconnected);
        channel_->DisableAll();

        connectioncallback_(shared_from_this());
    }
    channel_->Remove();
}

const char* TcpConnection::StateToString() const
{
    switch (state_)
    {
    case StateE::kDisconnected:
        return "kDisconnected";
    case StateE::kConnecting:
        return "kConnecting";
    case StateE::kConnected:
        return "kConnected";
    case StateE::kDisconnecting:
        return "kDisconnecting";
    default:
        return "unknown state";
    }
}

void TcpConnection::Init()
{
    channel_->set_readcallback(bind(&TcpConnection::HandleRead, shared_from_this(), placeholders::_1));
    channel_->set_writecallback(bind(&TcpConnection::HandleWrite, shared_from_this()));
    channel_->set_closecallback(bind(&TcpConnection::HandleClose, shared_from_this()));
    channel_->EnableRead();
}

void TcpConnection::ShutdownInLoop()
{
    loop_->AssertInLoopThread();
    if (!channel_->IsWriting())
    {
        // we are not writing
        ::shutdown(channel_->get_FD(), SHUT_WR);
    }
}



