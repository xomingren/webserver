#pragma once

#include <atomic>
#include <memory>//for enbale_shared_from_this
#include <string_view>

#include "buffer_class.h"
#include "channel_class.h"
#include "define.h"
#include "noncopyable_class.h"
#include "timestamp_class.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
						noncopyable
{
public:
	TcpConnection(EventLoop* loop, const std::string& nameArg, FD socketfd);
	~TcpConnection();

	EventLoop* get_loop() const { return loop_; }
	const std::string& get_name() const { return name_; }

	void HandleRead(Timestamp receivetime);
	void HandleClose();
	void HandleWrite();
	void OnConnectEstablished();
	void ConnectDestroyed();

	void Send(const void* message, size_t len);
	void Send(const std::string_view& message);
	void Send(Buffer* message);  // this one will swap data
	void SendInLoop(const std::string_view& message);//problemmark string_view
	void SendInLoop(const void* message, size_t len);

	void Shutdown(); // NOT thread safe, no simultaneous calling

	const char* StateToString() const;
	
	void set_messagecallback(MessageCallBack messagecallback)
	{ messagecallback_ = messagecallback; }
	void set_connectioncallback(ConnectionCallBack connectioncallback)
	{ connectioncallback_ = connectioncallback; }
	void set_writecompletecallback(WriteCompleteCallback cb)
	{ writecompletecallback_ = std::move(cb); }
	void set_highwatermarkcallback(HighWaterMarkCallback cb, size_t highWaterMark)
	{ highwatermarkcallback_ = cb; highwatermark_ = highWaterMark; }

	void Init();

	/// Internal use only.
	void set_closecallback(const CloseCallback& cb)
	{ closecallback_ = cb; }

private:
	enum class StateE :unsigned char
	{
		kDisconnected, 
		kConnecting, 
		kConnected,
		kDisconnecting 
	};
	std::atomic<StateE> state_;

	void ShutdownInLoop();
	void set_state(StateE state)
	{ state_.store(state); }

	//FD socketfd_;
	EventLoop* loop_;
	const std::string name_;
	std::unique_ptr<Channel> channel_;
	size_t highwatermark_;
	Buffer inbuf_;
	Buffer outbuf_;// FIXME: use list<Buffer> as output buffer.
	MessageCallBack messagecallback_;
	ConnectionCallBack connectioncallback_;
	WriteCompleteCallback writecompletecallback_;//called when outbuf_ is empty
	HighWaterMarkCallback highwatermarkcallback_;//called when wrote too many
	CloseCallback closecallback_;
};

