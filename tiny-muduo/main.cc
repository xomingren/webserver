#include "eventloop _class.h"
#include "echo_servevr_class.h"
#include "tcp_server_class.h"

int main(int args, char** argv)
{
    EventLoop loop;
    EchoServer echoserver(&loop);
    echoserver.Start();
    loop.Loop();
    return 0;
}