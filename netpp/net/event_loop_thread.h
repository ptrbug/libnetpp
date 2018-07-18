#pragma once
#include <netpp/net/event_loop.h>
#include <mutex>
#include <thread>
#include <boost/thread/latch.hpp>

namespace netpp {
class EventLoopThread : public ServerStatus, public boost::noncopyable
{
public:
	enum { kOK = 0 };

	typedef std::function<int()> Functor;

	EventLoopThread();
	~EventLoopThread();

	bool start(bool wait_thread_started = true,
		Functor pre = Functor(),
		Functor post = Functor());

	void stop(bool wait_thread_exit = false);
	void join();

	void setName(const std::string& name);
	const std::string& getName() const;
	EventLoop* loop() const;
	std::thread::id tid() const;
	bool isRunning() const;

private:
	void run(const Functor& pre, const Functor& post);

private:
	std::shared_ptr<EventLoop> event_loop_;
	std::mutex mutex_;
	std::shared_ptr<std::thread> thread_;
	std::string name_;
	boost::latch latch_ = {1};
};
}