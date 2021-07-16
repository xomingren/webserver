#include "tcp_connection _class.h"

#include <errno.h>//for errno
#include <unistd.h>//for read write close
#include <iostream>//for cout

#include <string.h> //for bzero and string

using namespace std;

TcpConnection::TcpConnection(EventLoop* loop, FD socketfd)
    : socketfd_(socketfd),
      loop_(loop)
{
    channel_ = new Channel(loop_, socketfd_); // Memory Leak !!!
    channel_->set_callbackfunc(OnEvent);
    channel_->EnableRead();
}

void TcpConnection::OnEvent(FD socketfd)
{
    ssize_t readlength;
    char line[kMaxLine];
    if (socketfd < 0)
    {
        cout << "EPOLLIN sockfd < 0 error " << endl;
        return;
    }
    bzero(line, kMaxLine);
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
        if (write(socketfd, line, readlength) != readlength)
            cout << "error: not finished one time" << endl;
        string str = line;
        cout << str << endl;
    }
}

