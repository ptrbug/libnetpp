#pragma once
#include <netpp/net/tcp_callbacks.h>
#include <netpp/net/event_loop.h>
#include <netpp/net/buffer.h>
#include <boost/asio.hpp>
#include <boost/any.hpp>
#include <queue>
#include <atomic>

namespace netpp {
using namespace boost::asio;

class PiecePool;
class TCPConn : public boost::noncopyable, public std::enable_shared_from_this<TCPConn>
{
public:
	TCPConn(EventLoop* loop
		, SocketPtr sock
		, uint64_t id);
	~TCPConn();

	EventLoop* loop() const {
		return loop_;
	}
	uint64_t id() const { return id_; }
	const ip::tcp::endpoint& localAddress() const { 
		return local_addr_; }
	const ip::tcp::endpoint& remoteAddress() const { 
		return remote_addr_; }
	bool isConnected() const { 
		return status_ == kConnected; }
	bool isDisconnecting() const { 
		return status_ == kDisconnecting; }
	bool isDisconnected() const { 
		return status_ == kDisconnected; }

	void send(const char* s);
	void send(const std::string& d);
	void send(const void* data, size_t len);	
	void send(Buffer* buffer);

	void close();
	void closeWithDelay(long seconds);
	void setTcpNoDelay(bool on);	

	void setContext(const boost::any& context) {
		context_ = context; }
	const boost::any& getContext() const {
		return context_;	}
	boost::any* getMutableContext()	{
		return &context_;	}

	void connectEstablished();

	void setConnectionCallback(const ConnectionCallback& cb) { connection_cb_ = cb; }
	void setMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }
	void setCloseCallback(CloseCallback&& cb) { close_cb_ = std::move(cb); }

private:
	enum StateE { kConnected, kDisconnecting, kDisconnected };
	class InputBuffer : public Buffer {
	public:
		void writePiece(Piece* item);
	};
	class OutputBuffer : public Buffer {
	public:
		Piece* readPiece();
		void writePieceQueue(Piece* queue_head);
	};

	void setState(StateE s) { status_ = s; }
	void sendInLoop(Piece* queue_head);
	void launchWrite();
	void handleWrite(Piece* piece, const boost::system::error_code &ec);
	void launchRead();
	void handleRead(Piece* piece, const boost::system::error_code &ec, size_t bytes_transferred);
	void handleError();
	void handleClose();

	EventLoop* loop_;
	const uint64_t id_;
	SocketPtr socket_;	
	ip::tcp::endpoint local_addr_;
	ip::tcp::endpoint remote_addr_;

	std::atomic<StateE> status_;
	bool is_sending_;
	OutputBuffer output_buffer_;
	InputBuffer input_buffer_;
	boost::any context_;
	DealTimerPtr delay_close_timer_;

	//callbacks
	ConnectionCallback connection_cb_;
	MessageCallback message_cb_;
	CloseCallback close_cb_;
};
}