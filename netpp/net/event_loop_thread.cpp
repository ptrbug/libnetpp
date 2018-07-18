#include <netpp/net/event_loop_thread.h>
#include <netpp/base/logging.h>

namespace netpp {
EventLoopThread::EventLoopThread()
	: event_loop_(new EventLoop) {
}

EventLoopThread::~EventLoopThread() {
	assert(isStopped());
	join();
}

bool EventLoopThread::start(bool wait_thread_started ,
	Functor pre,
	Functor post) {
	status_ = kStarting;

	assert(thread_.get() == nullptr);
	thread_.reset(new std::thread(std::bind(&EventLoopThread::run, this, pre, post)));

	if (wait_thread_started) {
		latch_.wait();
		assert(status_ == kRunning);
	}
	return true;
}

void EventLoopThread::stop(bool wait_thread_exit) {
	assert(status_ == kRunning && isRunning());
	status_ = kStopping;
	event_loop_->stop();

	if (wait_thread_exit) {
		while (!isStopped()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		join();
	}
}

void EventLoopThread::join() {
	std::lock_guard<std::mutex> guard(mutex_);
	if (thread_ && thread_->joinable())	{
		try	{
			thread_->join();
		}
		catch (const std::system_error& e) {
			LOG_ERROR << "Caught a system_error:" << e.what() << " code=" << e.code();
		}
		thread_.reset();
	}
}

void EventLoopThread::setName(const std::string& name) {
	name_ = name;
}

const std::string& EventLoopThread::getName() const {
	return name_;
}

EventLoop* EventLoopThread::loop() const {
	return event_loop_.get();
}

std::thread::id EventLoopThread::tid() const {
	if (thread_) {
		return thread_->get_id();
	}
	return std::thread::id();
}

bool EventLoopThread::isRunning() const {
	return event_loop_->isRunning();
}

void EventLoopThread::run(const Functor& pre, const Functor& post) {
	if (name_.empty()) {
		std::ostringstream os;
		os << "thread-" << std::this_thread::get_id();
		name_ = os.str();
	}

	auto fn = [this, pre]()	{
		status_ = kRunning;
		latch_.count_down();
		if (pre) {
			auto rc = pre();
			if (rc != kOK) {
				event_loop_->stop();
			}
		}
	};

	event_loop_->queueInLoop(std::move(fn));
	event_loop_->start();

	if (post) {
		post();
	}

	assert(event_loop_->isStopped());
	status_ = kStopped;
}
}