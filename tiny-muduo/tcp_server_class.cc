#include "tcp_server_class.h"

using namespace std;

TcpServer* TcpServer::ptr_this = nullptr;

TcpServer::TcpServer()
{
    ptr_this = this;
}
SocketFD TcpServer::CreateSocketAndListenOrDie()
{
    int optionval = 1;
    SocketFD listensocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    fcntl(listensocket, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(int));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if (-1 == bind(listensocket, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl;
    }

    if (-1 == listen(listensocket, kMaxListenFd))//2nd param is the queue size kernel keeped that done with 3 handshake and haven't been accepted
    {
        cout << "listen error, errno:" << errno << endl;
    }
    return listensocket;
}
void TcpServer::OnEvent(FD sockfd)
{
    cout << "OnEvent:" << sockfd << endl;
    if (sockfd == ptr_this->listenfd_)
    {
        SocketFD connectfd;
        sockaddr_in cliaddr;
        socklen_t clilen = sizeof(sockaddr_in);
        connectfd = accept(ptr_this->listenfd_, reinterpret_cast<sockaddr*>(&cliaddr), &clilen);
        if (connectfd > 0)
        {
            cout << "new connection from "
                << "[" << inet_ntoa(cliaddr.sin_addr)
                << ":" << ntohs(cliaddr.sin_port) << "]"
                << " new socket fd:" << connectfd
                << endl;
        }
        else
        {
            cout << "accept error, connfd:" << connectfd
                << " errno:" << errno << endl;
        }
        fcntl(connectfd, F_SETFL, O_NONBLOCK); //no-block io

        // Memory Leak !!!
        //fucntion EnableRead() will fill its own *this pointer to a epoll_event ,then combain with epollfd_ and connectfd
        //do not use local variant
        Channel* channel = new Channel(ptr_this->epollfd_, connectfd);
        channel->set_callbackfunc(OnEvent);
        channel->EnableRead();
    }
    else
    {
        ssize_t readlength;
        char line[kMaxLine];
        if (sockfd < 0)
        {
            cout << "EPOLLIN sockfd < 0 error " << endl;
            return;
        }
        bzero(line, kMaxLine);
        if ((readlength = read(sockfd, line, kMaxLine)) < 0)
        {
            if (errno == ECONNRESET)
            {
                cout << "ECONNREST closed socket fd:" << sockfd << endl;
                close(sockfd);
            }
        }
        else if (readlength == 0)
        {
            cout << "read 0 closed socket fd:" << sockfd << endl;
            close(sockfd);
        }
        else
        {
            if (write(sockfd, line, readlength) != readlength)
                cout << "error: not finished one time" << endl;
            string str = line;
            cout << str << endl;
        }
    }
}

void TcpServer::Start()
{
    epollfd_ = epoll_create(1);
    if (epollfd_ <= 0)
        cout << "epoll_create error, errno:" << epollfd_ << endl;
    listenfd_ = CreateSocketAndListenOrDie();

    Channel* channel = new Channel(epollfd_, listenfd_);
    channel->set_callbackfunc(OnEvent);
    channel->EnableRead();

    for (;;)
    {
        vector<Channel*> channels;
        channels.reserve(kMaxEvents);
        cout << "epoll waiting..." << endl;
        FD fds = epoll_wait(epollfd_, events_, kMaxEvents, -1);
        if (fds == -1)
        {
            cout << "epoll_wait error, errno:" << errno << endl;
            break;
        }
        for (int i = 0; i < fds; ++i)
        {
            Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
            channel->set_revent(events_[i].events);//sign epollin|epollet event
            channels.push_back(channel);
        }

        for (const auto& it : channels)
        {
            it->HandleEvent();
        }
    }
}
