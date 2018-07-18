// echo_client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../../proto/echo.pb.h"

using namespace netpp;

class EchoClient
{
public:
	EchoClient(EventLoop* loop)
		: loop_(loop)
		, dispatcher_(std::bind(&EchoClient::onUnknownMessage, this, std::placeholders::_1, std::placeholders::_2))
		, codec_(std::bind(&ProtobufDispatcher::onProtobufMessage, &dispatcher_, std::placeholders::_1, std::placeholders::_2)){

		fix_data_.assign(1024, 'a');
		
	}

	void start(const ip::tcp::endpoint& remoteAddr, int connection_cnt)
	{
		for (int i = 0; i < connection_cnt; ++i)
		{
			std::shared_ptr<TCPClient> client(new TCPClient(loop_, remoteAddr, "TcpClientTest"));
			tcpClients_[i] = client;

			dispatcher_.registerMessageCallback<Echo::Response>(
				std::bind(&EchoClient::onResponse, this, std::placeholders::_1, std::placeholders::_2));
			client->setConnectionCallback(
				std::bind(&EchoClient::onConnectionCallback, this, std::placeholders::_1));
			client->setMessageCallback(
				std::bind(&ProtobufCodec::onMessage, &codec_, std::placeholders::_1, std::placeholders::_2));
			client->connect();
		}

		loop_->runEvery(boost::posix_time::seconds(1), std::bind(&EchoClient::showInfo, this));

	}

private:
	void onConnectionCallback(const TCPConnPtr& conn)
	{
		if (conn->isConnected()) {
			connection_cnt_++;
			Echo::Request request;
			request.set_id(request_id_++);
			request.set_ask(fix_data_);
			codec_.send(conn, request);
		}
		else if (conn->isDisconnecting()) {
			connection_cnt_--;
		}
	}

	void onUnknownMessage(const TCPConnPtr& conn, const MessagePtr& message) {
		//LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
		conn->close();
	}

	void onResponse(const TCPConnPtr& conn, const std::shared_ptr<Echo::Response>& message) {
		//LOG_INFO << "onResponse: " << message->id() << "," << message->answer();
		last_second_recv_cnt_++;
		total_recv_cnt_++;

		Echo::Request request;
		request.set_id(request_id_++);
		request.set_ask(message->answer());
		codec_.send(conn, request);
	}

	void showInfo()	{
		std::cout << "connection_cnt:" << connection_cnt_
			<< " last_second_recv_cnt: " << last_second_recv_cnt_
			<< " total_recv_cnt: " << total_recv_cnt_ << std::endl;
		last_second_recv_cnt_ = 0;
	}

private:
	typedef std::map<uint64_t, std::shared_ptr<TCPClient>> TcpClientMap;
	EventLoop* loop_;
	ProtobufDispatcher dispatcher_;
	ProtobufCodec codec_;
	TcpClientMap tcpClients_;
	std::atomic<uint64_t> request_id_ = {0};
	std::atomic<int> connection_cnt_ = {0};
	std::atomic<int64_t> last_second_recv_cnt_ = {0};
	std::atomic<int64_t> total_recv_cnt_ = { 0 };

	std::string fix_data_;
};

int _tmain(int argc, _TCHAR* argv[])
{
	EventLoop loop;

	EchoClient mgr_test(&loop);
	ip::tcp::endpoint remoteAddr(ip::address::from_string("127.0.0.1"), 8765);
	int connection_cnt = 10000;

	std::cout << "client running:" << std::endl
		<< "remote: "
		<< "ip: " << remoteAddr.address().to_string()
		<< " port: " << remoteAddr.port()
		<< " connection cnt: " << connection_cnt
		<< std::endl;

	mgr_test.start(remoteAddr, connection_cnt);
	loop.start();

	return 0;
}