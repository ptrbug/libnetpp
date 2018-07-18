#include <netpp/net/tcp_client.h>

namespace netpp {
std::atomic<uint64_t> id = 1;

TCPClient::TCPClient(EventLoop* loop
	, const ip::tcp::endpoint& remoteAddr
	, const std::string& name)
	: loop_(loop)
	, remote_addr_(remoteAddr)
	, name_(name)
	, auto_reconnect_(true)
	, reconnect_interval_seconds_(3)
	, is_connecting_(false) {
}

TCPClient::~TCPClient() {
}

void TCPClient::bind(const ip::tcp::endpoint& localAddr) {
	local_addr_ = localAddr;
}

void TCPClient::connect() {
	loop_->runInLoop([this](){
		if (is_connecting_ || (conn_ && conn_->isConnected())) {
			return;
		}
		is_connecting_ = true;
		SocketPtr sock(new ip::tcp::socket(loop_->getIoService()));		
		if (local_addr_.port() != 0) {
			sock->open(remote_addr_.protocol());
			sock->bind(local_addr_);
		}
		sock->async_connect(remote_addr_
			, std::bind(&TCPClient::newConnection, this, sock, std::placeholders::_1));
	});
}

void TCPClient::disconnect() {
	loop_->runInLoop([this](){
		auto_reconnect_ = false;
		if (conn_) {
			assert(!conn_->isDisconnected() && !conn_->isDisconnecting());
			conn_->close();
		}
	});
}

void TCPClient::newConnection(SocketPtr sock, const boost::system::error_code &ec) {
	is_connecting_ = false;

	if (!ec) {
		TCPConnPtr conn(new TCPConn(loop_, sock, id++));
		conn->setConnectionCallback(connection_cb_);
		conn->setMessageCallback(message_cb_);
		conn->setCloseCallback(std::bind(&TCPClient::removeConnection, this, std::placeholders::_1));
		loop_->runInLoop(std::bind(&TCPConn::connectEstablished, conn));

		{
			std::unique_lock<std::mutex> lock(mutex_);
			conn_ = conn;
		}
	}
	else {
		TCPConnPtr conn(new TCPConn(loop_, sock, 0));
		connection_cb_(conn);

		if (auto_reconnect_.load()) {
			loop_->runAfter(boost::posix_time::seconds(reconnect_interval_seconds_), std::bind(&TCPClient::reconnect, this));
		}
	}	
}

void TCPClient::removeConnection(const TCPConnPtr& conn) {
	assert(conn.get() == conn_.get());
	assert(loop_->isInLoopThread());
	conn_.reset();
	if (auto_reconnect_.load())	{
		reconnect();
	}
}

void TCPClient::reconnect() {
	connect();
}
}