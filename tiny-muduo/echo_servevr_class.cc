#include "echo_servevr_class.h"

#include <functional>
#include <iostream>

using namespace std;

EchoServer::EchoServer(EventLoop* loop)
	: loop_(loop),
	  tcpserver_(loop)
{
	tcpserver_.set_messagecallback(bind(&EchoServer::OnMessage,this, placeholders::_1,placeholders::_2));
	tcpserver_.set_connectioncallback(bind(&EchoServer::OnConnection, this, placeholders::_1));
}

void EchoServer::OnConnection(TcpConnection* tcpconnection)
{
	cout << "onconnection" << endl;;
}

void EchoServer::OnMessage(TcpConnection* tcpconnection, const string& data)
{
	cout << data << endl;
	tcpconnection->Send(data);
}
