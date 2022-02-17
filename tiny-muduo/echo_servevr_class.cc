#include "commonfunction.h"
#include "echo_servevr_class.h"

#include <functional>
#include <iostream>
#include <memory.h>

using namespace std;

EchoServer::EchoServer(EventLoop* loop)
	: loop_(loop),
	  tcpserver_(loop)
{
	tcpserver_.set_messagecallback(bind(&EchoServer::OnMessage,this, placeholders::_1,placeholders::_2));
	tcpserver_.set_connectioncallback(bind(&EchoServer::OnConnection, this, placeholders::_1));
	tcpserver_.set_writecompletecallback(bind(&EchoServer::OnWriteComplete, this, placeholders::_1));
	tcpserver_.set_highwatermarkcallback(bind(&EchoServer::OnHighWaterMark, this, placeholders::_1, placeholders::_2), 64 * 1024);
}

void EchoServer::OnConnection(TcpConnection* tcpconnection)
{
	cout << "onconnection" << endl;;
}

void EchoServer::OnMessage(TcpConnection* tcpconnection, Buffer* buf)
{
	//while (buf->ReadableBytes() > kMessageLength)
	{
		size_t changebufflength = buf->ReadableBytes() * 2;
		string message = buf->RetrieveAllAsString();		;
		char utf8[changebufflength];
		memset(utf8, 0, sizeof utf8);
		g2u(const_cast<char*>(message.c_str()), sizeof utf8, utf8, sizeof utf8);
		string showstr(utf8);
		cout << showstr << endl;
		tcpconnection->Send(message);
	}	
}

void EchoServer::OnWriteComplete(TcpConnection* tcpconnection)
{
	cout << "onWriteComplete" << endl;
}
void EchoServer::OnHighWaterMark(TcpConnection* conn, size_t len)
{
	cout << "HighWaterMark " << len;
}