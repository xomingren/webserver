#pragma once

#include <any>
#include <atomic>
#include <memory>//for enbale_shared_from_this
#include <string_view>

#include "buffer_class.h"
#include "define.h"
#include "noncopyable_class.h"

class Channel;
class EventLoop;

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
	void OnConnectDestroyed();

	bool Connected() const { return state_ == StateE::kConnected; }

	void Send(const void* message, size_t len);
	void Send(const std::string_view& message);
	void Send(Buffer* message);  // this one will swap data
	void SendInLoop(const std::string_view& message);//problemmark string_view
	void SendInLoop(const void* message, size_t len);

	void Shutdown(); // NOT thread safe, no simultaneous calling

	void ForceClose();

	void ForceCloseInLoop();

	const char* StateToString() const;
	
	void set_messagecallback(MessageCallback messagecallback)
	{ messagecallback_ = messagecallback; }
	void set_connectioncallback(ConnectionCallback connectioncallback)
	{ connectioncallback_ = connectioncallback; }
	void set_writecompletecallback(WriteCompleteCallback cb)
	{ writecompletecallback_ = std::move(cb); }
	void set_highwatermarkcallback(HighWaterMarkCallback cb, size_t highWaterMark)
	{ highwatermarkcallback_ = cb; highwatermark_ = highWaterMark; }
	void set_context_(const std::any& context)
	{ context_ = context; }
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
	MessageCallback messagecallback_;
	ConnectionCallback connectioncallback_;
	WriteCompleteCallback writecompletecallback_;//called when outbuf_ is empty
	HighWaterMarkCallback highwatermarkcallback_;//called when wrote too many
	CloseCallback closecallback_;
	std::any context_;
};

