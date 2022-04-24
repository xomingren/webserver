#include <atomic>
#include <functional>//for std::bind
#include <iconv.h>
#include <iostream>
#include <memory.h>//for memset
#include <string>

#include "../../tiny_muduo/buffer_class.h"
#include "../../tiny_muduo/codec_class.h"
#include "../../tiny_muduo/commonfunction.h"
#include "../../tiny_muduo/eventloop_class.h"
#include "../../tiny_muduo/tcpconnection_class.h"
#include "../../tiny_muduo/tcpserver_class.h"

#include "../../tiny_muduo/log.h"

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
	void OnConnection(const TcpConnectionPtr& tcpconnection);
	//void OnMessage(const TcpConnectionPtr& tcpconnection, Buffer* buf, Timestamp );
	void OnMessage(const TcpConnectionPtr& tcpconnection, const std::string& buf, Timestamp);
	void OnWriteComplete(const TcpConnectionPtr& tcpconnection);
	void OnHighWaterMark(const TcpConnectionPtr& tcpconnection, size_t len);
	void PrintThroughput();

private:
	int code_convert(const char* from_charset, const char* to_charset, char* inbuf, size_t inlen, char* outbuf, size_t outlen)
	{
		iconv_t cd;
		char** pin = &inbuf;
		char** pout = &outbuf;


		cd = iconv_open(to_charset, from_charset);
		if (cd == 0)
			return -1;
		memset(outbuf, 0, outlen);
		iconv(cd, pin, &inlen, pout, &outlen);
		//if (iconv(cd, pin, &inlen, pout, &outlen) == -1)
		//	return -1;
		iconv_close(cd);
		**pout = '\0';
		return 0;
	}
	int u2g(char* inbuf, size_t inlen, char* outbuf, size_t outlen)
	{
		return code_convert("utf-8", "gb2312", inbuf, inlen, outbuf, outlen);
	}
	int g2u(char* inbuf, size_t inlen, char* outbuf, size_t outlen)
	{
		return code_convert("gb2312", "utf-8", inbuf, inlen, outbuf, outlen);
	}

	EventLoop* loop_;
	TcpServer tcpserver_;
	LengthHeaderCodec codec_;

	std::atomic<int64_t> transferred_;
	std::atomic<int64_t> receivedmessages_;
	int64_t oldcounter_;
	Timestamp starttime_;
	int32_t counter;
};
EchoServer::EchoServer(EventLoop* loop)
	: loop_(loop),
	  tcpserver_(loop),
	  codec_(std::bind(&EchoServer::OnMessage, this, placeholders::_1, placeholders::_2, placeholders::_3)),
      oldcounter_(0),
	  starttime_(Timestamp::Now()),
	  counter(0)
{
	tcpserver_.SetThreadNum(4);
	//tcpserver_.set_messagecallback(bind(&EchoServer::OnMessage,this, placeholders::_1,placeholders::_2, placeholders::_3));
	tcpserver_.set_messagecallback(std::bind(&LengthHeaderCodec::OnMessage, &codec_, placeholders::_1, placeholders::_2, placeholders::_3));
	tcpserver_.set_connectioncallback(bind(&EchoServer::OnConnection, this, placeholders::_1));
	tcpserver_.set_writecompletecallback(bind(&EchoServer::OnWriteComplete, this, placeholders::_1));
	tcpserver_.set_highwatermarkcallback(bind(&EchoServer::OnHighWaterMark, this, placeholders::_1, placeholders::_2), 64 * 1024);
	loop_->RunEvery(3.0, std::bind(&EchoServer::PrintThroughput, this));
}

void EchoServer::OnConnection(const TcpConnectionPtr& tcpconnection)
{
	LOG_INFO << "OnConnection().";
}

//for recieve buf directly
//void EchoServer::OnMessage(const TcpConnectionPtr& tcpconnection, Buffer* buf, Timestamp time)
//{
//	//while (buf->ReadableBytes() > kMessageLength)
//	{
//		//for PrintThroughput
//		size_t len = buf->ReadableBytes();
//		transferred_.fetch_add(len);
//		++receivedmessages_;
//	
//		size_t changebufflength = len * 2;
//		string message = buf->RetrieveAllAsString();		;
//		char utf8[changebufflength];
//		memset(utf8, 0, sizeof utf8);
//		g2u(const_cast<char*>(message.c_str()), sizeof utf8, utf8, sizeof utf8);
//		string showstr(utf8);
//		cout << showstr << endl;
//		tcpconnection->Send(message);
//	}	
//}

//recieve from codec
void EchoServer::OnMessage(const TcpConnectionPtr& tcpconnection, const std::string& message, Timestamp time)
{
	++counter;
	if (counter > 10)
	{
		loop_->Quit();
		return;
	}
	//for PrintThroughput
	size_t len = message.size();
	transferred_.fetch_add(len);
	++receivedmessages_;

	size_t changebufflength = len * 2;
	char utf8[changebufflength];
	memset(utf8, 0, sizeof utf8);
	g2u(const_cast<char*>(message.c_str()), sizeof utf8, utf8, sizeof utf8);
	string showstr(utf8);
	cout << showstr << endl;
	codec_.Send(tcpconnection.get(), message);
}

void EchoServer::OnWriteComplete(const TcpConnectionPtr& tcpconnection)
{
	LOG_INFO << "OnWriteComplete().";
}

void EchoServer::OnHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
	LOG_INFO << "OnHighWaterMark().";
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
	char* curdir;
	curdir = getcwd(NULL, 0);
	tiny_muduo_log::Initialize(tiny_muduo_log::GuaranteedLogger(), string(curdir) + "/", "tinymuduolog", 1);

	EventLoop loop;
	EchoServer echoserver(&loop);
	echoserver.Start();
	loop.Loop();
	return 0;
}