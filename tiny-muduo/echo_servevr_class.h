#pragma once

#include<atomic>
#include <string>

#include "buffer_class.h"
#include "eventloop_class.h"
#include "tcp_connection_class.h"
#include "tcp_server_class.h"


class EchoServer
{
public:
    EchoServer(EventLoop* loop);
    EchoServer(const EchoServer&) = delete;
    EchoServer& operator =(const EchoServer&) = delete;
    ~EchoServer() = default;

    void Start()
    { tcpserver_.Start(); }
    void OnConnection(TcpConnection* tcpconnection);
    void OnMessage(TcpConnection* tcpconnection,Buffer* buf);
    void OnWriteComplete(TcpConnection* tcpconnection);
    void OnHighWaterMark(TcpConnection* tcpconnection, size_t len);
    void PrintThroughput();
private:
    EventLoop* loop_;
    TcpServer tcpserver_;

    std::atomic<int64_t> transferred_;
    std::atomic<int64_t> receivedmessages_;
    int64_t oldcounter_;
    Timestamp starttime_;
};

