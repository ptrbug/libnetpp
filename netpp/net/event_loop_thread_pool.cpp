#include <netpp/net/event_loop_thread_pool.h>

namespace netpp {
EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, uint32_t thread_num)
	: base_loop_(base_loop)
	, thread_num_(thread_num) {
}

EventLoopThreadPool::~EventLoopThreadPool() {
	join();
	threads_.clear();
}

bool EventLoopThreadPool::start(bool wait_thread_started) {
	status_.store(kStarting);

	if (thread_num_ == 0) {
		status_.store(kRunning);
		return true;
	}

	std::shared_ptr<std::atomic<uint32_t>> started_count(new std::atomic<uint32_t>(0));
	std::shared_ptr<std::atomic<uint32_t>> exited_count(new std::atomic<uint32_t>(0));
	for (uint32_t i = 0; i < thread_num_; ++i) {
		auto prefn = [this, started_count]() {
			this->onThreadStarted(started_count->fetch_add(1) + 1);
			return EventLoopThread::kOK;
		};

		auto postfn = [this, exited_count]() {
			this->onThreadExited(exited_count->fetch_add(1) + 1);
			return EventLoopThread::kOK;
		};

		EventLoopThreadPtr t(new EventLoopThread());
		if (!t->start(wait_thread_started, prefn, postfn)) {
			return false;
		}

		std::stringstream ss;
		ss << "EventLoopThreadPool-thread-" << i << "th";
		t->setName(ss.str());
		threads_.push_back(t);
	}
	
	if (wait_thread_started) {
		while (!isRunning()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		assert(status_.load() == kRunning);
	}

	return true;
}

void EventLoopThreadPool::stop(bool wait_thread_exited) {
	stop(wait_thread_exited, DoneCallback());
}

void EventLoopThreadPool::stop(DoneCallback fn) {
	stop(false, fn);
}

void EventLoopThreadPool::join() {
	for (auto &t : threads_) {
		t->join();
	}
	threads_.clear();
}

EventLoop* EventLoopThreadPool::getNextLoop() {
	EventLoop* loop = base_loop_;

	if (isRunning() && !threads_.empty()) {

		int64_t next = next_.fetch_add(1);
		next = next % threads_.size();
		loop = (threads_[(size_t)next])->loop();
	}

	return loop;
}

void EventLoopThreadPool::stop(bool wait_thread_exit, DoneCallback fn) {
	status_.store(kStopping);

	if (thread_num_ == 0) {
		status_.store(kStopped);

		if (fn) {
			fn();
		}
		return;
	}

	stopped_cb_ = fn;

	for (auto &t : threads_) {
		t->stop();
	}

	auto is_stopped_fn = [this]() {
		for (auto &t : this->threads_) {
			if (!t->isStopped()) {
				return false;
			}
		}
		return true;
	};

	if (thread_num_ > 0 && wait_thread_exit) {
		while (!is_stopped_fn()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

	status_.store(kStopped);
}

void EventLoopThreadPool::onThreadStarted(uint32_t count) {
	if (count == thread_num_) {
		status_.store(kRunning);
	}
}

void EventLoopThreadPool::onThreadExited(uint32_t count) {
	if (count == thread_num_) {
		status_.store(kStopped);
		if (stopped_cb_) {
			stopped_cb_();
			stopped_cb_ = DoneCallback();
		}
	}
}
}