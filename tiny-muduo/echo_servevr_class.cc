#include "echo_servevr_class.h"

#include <atomic>
#include <functional>//for std::bind
#include <iostream>//for cout
#include <memory.h>//for memset
#include <string>

#include "buffer_class.h"
#include "commonfunction.h"
#include "eventloop_class.h"
#include "tcp_connection_class.h"
#include "tcp_server_class.h"

using namespace std;

class EchoServer
{
public:
	EchoServer(EventLoop* loop);
	EchoServer(const EchoServer&) = delete;
	EchoServer& operator =(const EchoServer&) = delete;
	~EchoServer() = default;

	void Start()
	{
		tcpserver_.Start();
	}
	void OnConnection(TcpConnection* tcpconnection);
	void OnMessage(TcpConnection* tcpconnection, Buffer* buf);
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
EchoServer::EchoServer(EventLoop* loop)
	: loop_(loop),
	  tcpserver_(loop),
      oldcounter_(0),
	  starttime_(Timestamp::Now())
{
	tcpserver_.set_messagecallback(bind(&EchoServer::OnMessage,this, placeholders::_1,placeholders::_2));
	tcpserver_.set_connectioncallback(bind(&EchoServer::OnConnection, this, placeholders::_1));
	tcpserver_.set_writecompletecallback(bind(&EchoServer::OnWriteComplete, this, placeholders::_1));
	tcpserver_.set_highwatermarkcallback(bind(&EchoServer::OnHighWaterMark, this, placeholders::_1, placeholders::_2), 64 * 1024);
	loop_->RunEvery(3.0, std::bind(&EchoServer::PrintThroughput, this));
}

void EchoServer::OnConnection(TcpConnection* tcpconnection)
{
	cout << "onconnection" << endl;;
}

void EchoServer::OnMessage(TcpConnection* tcpconnection, Buffer* buf)
{
	//while (buf->ReadableBytes() > kMessageLength)
	{
		//for PrintThroughput
		size_t len = buf->ReadableBytes();
		transferred_.fetch_add(len);
		++receivedmessages_;
	
		size_t changebufflength = len * 2;
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

void EchoServer::PrintThroughput()
{
	Timestamp endtime = Timestamp::Now();
	int64_t newcounter = transferred_.load();
	int64_t bytes = newcounter - oldcounter_;
	int64_t msgs = receivedmessages_.exchange(0);
	double time = timeDifference(endtime, starttime_);
	printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
		static_cast<double>(bytes) / time / 1024 / 1024,
		static_cast<double>(msgs) / time / 1024,
		static_cast<double>(bytes) / static_cast<double>(msgs));

	oldcounter_ = newcounter;
	starttime_ = endtime;
}

int main(int args, char** argv)
{
	EventLoop loop;
	EchoServer echoserver(&loop);
	echoserver.Start();
	loop.Loop();
	return 0;
}