#include "tcp_server_class.h"
using namespace std;
Socket TcpServer::CreateSocketAndListenOrDie()
{
    int optionval = 1;
    Socket listensocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    fcntl(listensocket, F_SETFL, O_NONBLOCK); //no-block io
    setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, &optionval, sizeof(int));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(11111);

    if (-1 == bind(listensocket, (sockaddr*)&servaddr, sizeof(servaddr)))
    {
        cout << "bind error, errno:" << errno << endl;
    }

    if (-1 == listen(listensocket, kMaxListenFd))
    {
        cout << "listen error, errno:" << errno << endl;
    }
    return listensocket;
}
void TcpServer::Start()
{
    epoll_event event, events[kMaxEvents];
    int listenfd, connfd, sockfd;
    ssize_t readlength;
    char line[kMaxLine];
    sockaddr_in cliaddr;
    socklen_t clilen = sizeof(sockaddr_in);
    int epollfd = epoll_create(1);
    if (epollfd < 0)
        cout << "epoll_create error, error:" << epollfd << endl;
    listenfd = CreateSocketAndListenOrDie();
    event.data.fd = listenfd;
    event.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);

    for (;;)
    {
        cout << "epoll waiting..." << endl;
        int fds = epoll_wait(epollfd, events, kMaxEvents, -1);
        if (fds == -1)
        {
            cout << "epoll_wait error, errno:" << errno << endl;
            break;
        }
        for (int i = 0; i < fds; i++)
        {
            if (events[i].data.fd == listenfd)
            {
                connfd = accept(listenfd, (sockaddr*)&cliaddr, (socklen_t*)&clilen);
                if (connfd > 0)
                {
                    cout << "new connection from "
                        << "[" << inet_ntoa(cliaddr.sin_addr)
                        << ":" << ntohs(cliaddr.sin_port) << "]"
                        << " accept socket fd:" << connfd
                        << endl;
                }
                else
                {
                    cout << "accept error, connfd:" << connfd
                        << " errno:" << errno << endl;
                }
                fcntl(connfd, F_SETFL, O_NONBLOCK); //no-block io
                epoll_event ev;
                ev.data.fd = connfd;
                ev.events = EPOLLIN;
                ev.events |= EPOLLET;//egde 
                if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev))
                    cout << "epoll_ctrl error, errno:" << errno << endl;
            }
            else if (events[i].events & EPOLLIN)
            {
                if ((sockfd = events[i].data.fd) < 0)
                {
                    cout << "EPOLLIN sockfd < 0 error " << endl;
                    continue;
                }
                bzero(line, kMaxLine);
                readlength = read(sockfd, line, kMaxLine);
                if (readlength < 0)
                {
                    if (errno == ECONNRESET)
                    {
                        cout << "ECONNREST closed socket fd:" << events[i].data.fd << endl;
                        close(sockfd);
                    }
                }
                else if (readlength == 0)
                {
                    cout << "read 0 closed socket fd:" << events[i].data.fd << endl;
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
    }
}
