#include "tcp_connection _class.h"

#include <errno.h>//for errno
#include <unistd.h>//for read write close
#include <iostream>//for cout
#include <memory.h>

//#include <string> //for string

using namespace std;

TcpConnection::TcpConnection(EventLoop* loop, FD socketfd)
    : socketfd_(socketfd),
      loop_(loop)
{
    channel_ = new Channel(loop_, socketfd_); // Memory Leak !!!
    channel_->set_callbackfunc(bind(&TcpConnection::OnRecieve,this, socketfd_));
    channel_->EnableRead();
}

void TcpConnection::OnRecieve(FD socketfd)
{
    ssize_t readlength;
    char line[kMaxLine];
    if (socketfd < 0)
    {
        cout << "EPOLLIN sockfd < 0 error " << endl;
        return;
    }
    memset(line, 0, kMaxLine);
    if ((readlength = read(socketfd, line, kMaxLine)) < 0)
    {
        if (errno == ECONNRESET)
        {
            cout << "ECONNREST closed socket fd:" << socketfd << endl;
            close(socketfd);
        }
    }
    else if (readlength == 0)
    {
        cout << "read 0 closed socket fd:" << socketfd << endl;
        close(socketfd);
    }
    else
    {
        string buf(line, kMaxLine);
        messagecallback_(this, buf);
    }
}

void TcpConnection::Send(const string& message)
{
    ssize_t n = ::write(socketfd_, message.c_str(), message.size());
    if (n != static_cast<int>(message.size()))
        cout << "write error ! " << message.size() - n << "bytes left" << endl;
}

void TcpConnection::OnConnectEstablished()
{
    connectioncallback_(this);
}

//void TcpConnection::setUser(IMuduoUser* user)
//{
//    _pUser = user;
//}


