// echo_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../proto/echo.pb.h"

using namespace netpp;

class EchoServer
{
public:
	EchoServer(EventLoop* loop, const ip::tcp::endpoint& listenAddr)
		: tcp_server_(loop, listenAddr, "EchoServer", 0)
		, dispatcher_(std::bind(&EchoServer::onUnknownMessage, this, std::placeholders::_1, std::placeholders::_2))
		, codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, std::placeholders::_1, std::placeholders::_2))
		, loop_(loop) {

		dispatcher_.registerMessageCallback<Echo::Request>(
			std::bind(&EchoServer::onRequest, this, std::placeholders::_1, std::placeholders::_2));
		tcp_server_.setConnectionCallback(
			std::bind(&EchoServer::onConnectionCallback, this, std::placeholders::_1));
		tcp_server_.setMessageCallback(
			std::bind(&ProtobufCodec::onMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
	}

	void start() {
		tcp_server_.start();
		loop_->runEvery(boost::posix_time::seconds(1), std::bind(&EchoServer::showInfo, this));
	}

private:
	void onConnectionCallback(const TCPConnPtr& conn)
	{
		if (conn->isConnected())
		{
			connection_cnt_++;
		}
		else if (conn->isDisconnecting())
		{
			connection_cnt_--;
		}
	}

	void onUnknownMessage(const TCPConnPtr& conn, const MessagePtr& message) {
		LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
		conn->close();
	}

	void onRequest(const TCPConnPtr& conn, const std::shared_ptr<Echo::Request>& message) {
		last_second_recv_cnt_++;
		total_recv_cnt_++;
		//LOG_INFO << "onRequest: " << message->id() << "," << message->ask();
		Echo::Response response;
		response.set_id(message->id());
		response.set_answer(message->ask());
		codec_.send(conn, response);		
	}

	void showInfo() {
		std::cout << "connection_cnt:" << connection_cnt_
		<< " last_second_recv_cnt: " << last_second_recv_cnt_
		<< " total_recv_cnt: " << total_recv_cnt_ << std::endl;
		last_second_recv_cnt_ = 0;
	}

private:	
	TCPServer tcp_server_;
	EventLoop* loop_;
	ProtobufDispatcher dispatcher_;
	ProtobufCodec codec_;
	std::atomic<int> connection_cnt_ = {0};
	std::atomic<int64_t> last_second_recv_cnt_ = { 0 };
	std::atomic<int64_t> total_recv_cnt_ = { 0 };
};

int _tmain(int argc, _TCHAR* argv[])
{
	EventLoop loop;
	ip::tcp::endpoint listenAddr(ip::address::from_string("0.0.0.0"), 8765);
	EchoServer echo_server(&loop, listenAddr);

	std::cout << "server running:" << std::endl
		<< "listen: "
		<< "ip: " << listenAddr.address().to_string()
		<< " port: " << listenAddr.port()
		<< std::endl;

	echo_server.start();
	loop.start();

	printf("server stoped!");
	return 0;
}

