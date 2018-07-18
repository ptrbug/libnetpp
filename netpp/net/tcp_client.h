#pragma once
#include <netpp/net/tcp_conn.h>
#include <netpp/net/event_loop.h>
#include <string>
#include <mutex>

namespace netpp {
using namespace boost::asio;
class TCPClient : public boost::noncopyable
{
public:
	TCPClient(EventLoop* loop
		, const ip::tcp::endpoint& remoteAddr
		, const std::string& name);
	~TCPClient();

	const std::string& getName() const	{ return name_; }

	void bind(const ip::tcp::endpoint& localAddr);
	void connect();
	void disconnect();

	bool getAutoReconnect() const {	return auto_reconnect_;	}
	void setAutoReconnect(bool v) {	auto_reconnect_ = v; }
	long getReconnectInterval() const { return reconnect_interval_seconds_; }
	void setReconnectInterval(long seconds) { reconnect_interval_seconds_ = seconds; }
	void setContext(const boost::any& context) { context_ = context; }
	const boost::any& getContext() const { return context_; }

	TCPConnPtr conn() const
	{
		std::unique_lock<std::mutex> lock(mutex_);
		return conn_;
	}

	void setConnectionCallback(ConnectionCallback&& cb)	{ connection_cb_ = std::move(cb); }
	void setMessageCallback(MessageCallback&& cb) { message_cb_ = std::move(cb); }

private:
	void newConnection(SocketPtr sock, const boost::system::error_code &ec);
	void removeConnection(const TCPConnPtr& conn);
	void reconnect();

	EventLoop* loop_;
	ip::tcp::endpoint local_addr_;
	ip::tcp::endpoint remote_addr_;
	const std::string name_;
	std::atomic<bool> auto_reconnect_;
	long reconnect_interval_seconds_;
	boost::any context_;

	mutable std::mutex mutex_; // this guard of connection_
	TCPConnPtr conn_;

	bool is_connecting_;

	//callbacks
	ConnectionCallback connection_cb_;
	MessageCallback message_cb_;
};
}





























