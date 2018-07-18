#include <netpp/net/tcp_server.h>
#include <boost/format.hpp>
#include <cstdio>

namespace netpp {
TCPServer::TCPServer(EventLoop* loop
	, const ip::tcp::endpoint& listenAddr
	, const std::string& name
	, uint32_t thread_num)
	: loop_(loop)
	, listen_addr_(listenAddr)
	, name_(name)
	, next_conn_id_(0)
	, connection_cb_(internal::defaultConnectionCallback)
	, message_cb_(internal::defaultMessageCallback)
	, verify_address_cb_(internal::defaultVerifyAddressCallback) {
	pool_.reset(new EventLoopThreadPool(loop_, thread_num));
}

TCPServer::~TCPServer() {
	assert(connections_.empty());
	assert(!acceptor_);
	if (pool_) {
		assert(pool_->isStopped());
		pool_.reset();
	}
}

bool TCPServer::start() {
	assert(status_ == kNull);
	status_.store(kStarting);
	bool ok = pool_->start(true);
	if (ok)	{
		auto f = [this]() {
			acceptor_.reset(new ip::tcp::acceptor(loop_->getIoService()));
			acceptor_->open(listen_addr_.protocol());
			acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_->bind(listen_addr_);
			acceptor_->listen();
			status_.store(kRunning);
			doAccept();
		};

		loop_->runInLoop(f);
	}

	return ok;
}

void TCPServer::stop(DoneCallback on_stopped_cb) {
	assert(status_ == kRunning);
	loop_->runInLoop(std::bind(&TCPServer::stopInLoop, this, on_stopped_cb));
}

void TCPServer::stopInLoop(DoneCallback on_stopped_cb) {
	assert(loop_->isInLoopThread());

	status_.store(kStopping);
	substatus_.store(kStoppingListener);

	boost::system::error_code ingore_ec;
	acceptor_->cancel(ingore_ec);
	acceptor_->close(ingore_ec);
	acceptor_.reset();

	if (connections_.empty()) {
		loop_->queueInLoop(std::bind(&TCPServer::stopInloopSafe, this, on_stopped_cb));
	}
	else {
		for (auto& c : connections_) {
			if (c.second->isConnected()) {
				c.second->close();
			}
		}
		stopped_cb_ = on_stopped_cb;
	}
}

void TCPServer::stopInloopSafe(DoneCallback on_stopped_cb) {
	assert(loop_->isInLoopThread());
	assert(substatus_.load() == kStoppingListener);

	stopThreadPool();

	if (on_stopped_cb) {
		on_stopped_cb();
	}
	
	status_.store(kStopped);
}

void TCPServer::stopThreadPool() {
	assert(loop_->isInLoopThread());
	assert(isStopping());
	substatus_.store(kStoppingThreadPool);
	pool_->stop(true);
	assert(pool_->isStopped());

	pool_->join();
	pool_.reset();

	substatus_.store(kSubStatusNull);
}

void TCPServer::doAccept() {
	EventLoop* io_loop = pool_->getNextLoop();
	SocketPtr sock(new ip::tcp::socket(io_loop->getIoService()));
	acceptor_->async_accept(*sock, std::bind(&TCPServer::newConnection, this, io_loop, sock, std::placeholders::_1));
}

void TCPServer::newConnection(EventLoop* io_loop, SocketPtr sock, const boost::system::error_code &ec) {	
	if (!ec) {
		bool veriy_success = verify_address_cb_(sock->remote_endpoint());
		if (veriy_success) {
			++next_conn_id_;

			TCPConnPtr conn(new TCPConn(io_loop, sock, next_conn_id_));
			connections_[next_conn_id_] = conn;
			conn->setConnectionCallback(connection_cb_);
			conn->setMessageCallback(message_cb_);
			conn->setCloseCallback(std::bind(&TCPServer::removeConnection, this, std::placeholders::_1));

			io_loop->runInLoop(std::bind(&TCPConn::connectEstablished, conn));
		}
	}

	if (ec != boost::asio::error::operation_aborted) {
		doAccept();
	}
}

void TCPServer::removeConnection(const TCPConnPtr& conn) {
	auto f = [this, conn]() {
		assert(this->loop_->isInLoopThread());
		this->connections_.erase(conn->id());
		if (isStopping() && this->connections_.empty()) {
			loop_->queueInLoop(std::bind(&TCPServer::stopInloopSafe, this, stopped_cb_));
		}
	};
	loop_->runInLoop(f);
}
}