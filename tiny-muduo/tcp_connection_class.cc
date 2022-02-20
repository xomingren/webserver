#include "tcp_connection_class.h"

#include <errno.h>//for errno
#include <unistd.h>//for read write close
#include <iostream>//for cout
//#include <memory.h>

#include "commonfunction.h"
#include "eventloop_class.h"

using namespace std;

TcpConnection::TcpConnection(EventLoop* loop, FD socketfd)
    : socketfd_(socketfd),
      loop_(loop),
      highwatermark_(64 * 1024 * 1024)//default value, but finally decide by lib user
{
    channel_ = new Channel(loop_, socketfd_); // Memory Leak !!!
    channel_->set_readcallbackfunc(bind(&TcpConnection::OnRecieve,this));
    channel_->set_writecallbackfunc(bind(&TcpConnection::OnWrite, this));
    channel_->EnableRead();
}

void TcpConnection::OnRecieve()
{
    SocketFD sockfd = channel_->get_FD();
    if (sockfd < 0)
    {
        cout << "EPOLLIN sockfd < 0 error " << endl;
        return;
    }
    const ssize_t readlength = inbuf_.ReadFd(sockfd);
    if (readlength > 0)
    {
        messagecallback_(/*shared_from_this()*/this, &inbuf_);//fixme
    }
    else
    {
        //TDOD:close tcpconnection
    }
}

void TcpConnection::OnWrite()
{
    SocketFD sockfd = channel_->get_FD();
    if (channel_->IsWriting())
    {
        ssize_t wrotelength = ::write(sockfd, outbuf_.Peek(), outbuf_.ReadableBytes());
        if (wrotelength > 0)
        {
            cout << "write " << wrotelength << " bytes data again" << endl;
            outbuf_.Retrieve(wrotelength);
            if (0 == outbuf_.ReadableBytes())//wrote done
            {
                channel_->DisableWrite();//remove EPOLLOUT
                if(writecompletecallback_)
                    loop_->QueueInLoop(bind(writecompletecallback_,this)); //invoke onWriteComplete
            }
            //if wrote undone,this func will be triggered again
        }
    }
}

// FIXME efficiency!!!
void TcpConnection::Send(Buffer* message)
{
    Send(outbuf_.Peek(), outbuf_.ReadableBytes());
    outbuf_.RetrieveAll();
}

void TcpConnection::Send(const string& message)
{
    Send(message.data(), message.size());
}

void TcpConnection::Send(const void* message, size_t len)
{
    ssize_t wrotelength = 0;
    size_t remaining = len;
    // 1.if nothing in output queue, try writing directly
    if (!channel_->IsWriting() && 0 == outbuf_.ReadableBytes())
    {
        wrotelength = ::write(socketfd_, message, len);
        if (wrotelength < 0) //wrote error
            cout << "write error" << endl;
        else //wrote succeed,not sure how many bytes wrote
        {
            remaining = len - wrotelength;
            if (0 == remaining && writecompletecallback_)//all bytes wrote,call complete
                loop_->QueueInLoop(bind(writecompletecallback_, this)); ////fixme with shared ptr
        }
    }
    //2.output queue isn't empty(means kernel cache cann't take more) 
    //3.or some bytes left unwrote,also means 2
    //for 2 and 3,append the remaining on outbufs' tail
    assert(remaining <= len);
    if (remaining > 0)
    {
        size_t oldlen = outbuf_.ReadableBytes();
        if (oldlen + remaining >= highwatermark_
            && oldlen < highwatermark_//ensure called in rising edge
            && highwatermarkcallback_)
        {
            loop_->QueueInLoop(bind(highwatermarkcallback_, this, oldlen + remaining));
        }
        outbuf_.Append(static_cast<const char*>(message) + wrotelength, remaining);
        if (!channel_->IsWriting())//add EPOLLOUT
        { 
            channel_->EnableWrite();
        }
        //when kernel has more space,it'll notice epollfd with epoll_wait
        //then trigger handlewrite()
    }
}

void TcpConnection::OnConnectEstablished()
{
    connectioncallback_(this);
}



