#pragma once
#include <netpp/net/server_status.h>
#include <functional>
#include <thread>
#include <boost/asio.hpp>

namespace netpp {
using namespace boost::asio;
typedef std::shared_ptr<deadline_timer> DealTimerPtr;

class EventLoop : public ServerStatus, public boost::noncopyable
{
public:	
	typedef std::function<void()> Functor;

	EventLoop();
	explicit EventLoop(io_service* io_service);
	~EventLoop();

	io_service& getIoService() const { return *io_service_; }

	void start();
	void stop();

	DealTimerPtr runAfter(double delay_ms, Functor&& f);
	DealTimerPtr runEvery(double delay_ms, Functor&& f);
	DealTimerPtr runAfter(const boost::posix_time::time_duration& delay, Functor&& f);	
	DealTimerPtr runEvery(const boost::posix_time::time_duration& delay, Functor&& f);
	void runInLoop(Functor&& handler);
	void queueInLoop(Functor&& handler);

	const std::thread::id& tid() const { return tid_;}
	bool isInLoopThread() const { return tid_ == std::this_thread::get_id(); }

private:
	void execTimerEvery(DealTimerPtr timer, boost::posix_time::time_duration delay, Functor f);

private:
	bool is_own_service_;
	io_service* io_service_;
	std::thread::id tid_;
	std::unique_ptr<io_service::work> work_;
};
}