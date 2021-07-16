#include "eventloop _class.h"
#include "tcp_server_class.h"

int main(int args, char** argv)
{
    EventLoop loop;
    TcpServer tcpserver(&loop);
    tcpserver.Start();
    loop.Loop();
    return 0;
}