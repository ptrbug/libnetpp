#pragma once
#include <netpp/net/tcp_conn.h>
#include <netpp/net/event_loop_thread_pool.h>
#include <boost/thread/latch.hpp>
#include <boost/scoped_ptr.hpp>
#include <atomic> 
#include <map>

namespace netpp {
using namespace boost::asio;

class TCPServer : public ServerStatus, public boost::noncopyable
{
public:
	typedef std::function<void()> DoneCallback;

	TCPServer(EventLoop* loop
		, const ip::tcp::endpoint& listenAddr
		, const std::string& name
		, uint32_t thread_num);
	~TCPServer();

	const std::string& getName() const { return name_; }
	
	bool start();
	void stop(DoneCallback on_stopped_cb);

	// Set connection callback, Not thread safe.
	void setConnectionCallback(ConnectionCallback&& cb){ 
		connection_cb_ = std::move(cb); }
	void setMessageCallback(MessageCallback&& cb){ 
		message_cb_ = std::move(cb); }
	void setVerifyAddressCallback(VerifyAddressCallback&& cb){ 
		verify_address_cb_ = std::move(cb); }

protected:
	typedef std::map<uint64_t, TCPConnPtr> ConnectionMap;
		
	void stopInLoop(DoneCallback on_stopped_cb);
	void stopInloopSafe(DoneCallback on_stopped_cb);
	void stopThreadPool();
	void doAccept();
	void newConnection(EventLoop* io_loop, SocketPtr sock, const boost::system::error_code &ec);
	void removeConnection(const TCPConnPtr& conn);

	EventLoop* loop_;
	ip::tcp::endpoint listen_addr_;
	const std::string name_;
	std::unique_ptr<ip::tcp::acceptor> acceptor_;
	uint64_t next_conn_id_;
	ConnectionMap connections_;	
	std::atomic_flag started_;
	std::shared_ptr<EventLoopThreadPool> pool_;

	//callbacks
	ConnectionCallback connection_cb_;
	MessageCallback message_cb_;
	VerifyAddressCallback verify_address_cb_;
	DoneCallback stopped_cb_;
};
}
