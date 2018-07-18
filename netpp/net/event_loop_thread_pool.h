#pragma once
#include <netpp/net/event_loop_thread.h>

namespace netpp {
class EventLoopThreadPool : public ServerStatus, public boost::noncopyable
{
public:
	typedef std::function<void()> DoneCallback;

	EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_num);
	~EventLoopThreadPool();

	bool start(bool wait_thread_started = false);
	void stop(bool wait_thread_exited = false);
	void stop(DoneCallback fn);

	void join();

	EventLoop* getNextLoop();

private:
	void stop(bool wait_thread_exit, DoneCallback fn);
	void onThreadStarted(uint32_t count);
	void onThreadExited(uint32_t count);

private:
	EventLoop* base_loop_;

	uint32_t thread_num_ = 0;
	std::atomic<int64_t> next_ = { 0 };

	DoneCallback stopped_cb_;

	typedef std::shared_ptr<EventLoopThread> EventLoopThreadPtr;
	std::vector<EventLoopThreadPtr> threads_;

};
}