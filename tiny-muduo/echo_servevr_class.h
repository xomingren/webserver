#pragma once

#include <string>

#include "eventloop _class.h"
#include "tcp_server_class.h"
#include "tcp_connection _class.h"

class EchoServer
{
public:
    EchoServer(EventLoop* loop);
    EchoServer(const EchoServer&) = delete;
    EchoServer& operator =(const EchoServer&) = delete;
    ~EchoServer() = default;

    void Start()
    { tcpserver_.Start(); }
    void  OnConnection(TcpConnection* tcpconnection);
    void  OnMessage(TcpConnection* tcpconnection,const std::string& data);
private:
    EventLoop* loop_;
    TcpServer tcpserver_;
};

