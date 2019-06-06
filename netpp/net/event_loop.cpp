#include <netpp/net/event_loop.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace netpp {
EventLoop::EventLoop()
	: is_own_service_(true)
	, io_service_(new io_service(1)) {
}

EventLoop::EventLoop(io_service* io_service)
	: is_own_service_(false)
	, io_service_(io_service) {
}

EventLoop::~EventLoop() {
	if (is_own_service_) {
		delete io_service_;
	}
}

void EventLoop::start() {	
	tid_ = std::this_thread::get_id();
	status_.store(kRunning);
	work_.reset(new io_service::work(*io_service_));
	io_service_->run();
	status_.store(kStopped);
}

void EventLoop::stop() {
	work_.reset();
	io_service_->stop();
}

DealTimerPtr EventLoop::runAfter(double delay_ms, Functor&& f) {
	boost::posix_time::microseconds delay((int64_t)(delay_ms * 1000));
	return runAfter(delay, std::move(f));
}

DealTimerPtr EventLoop::runEvery(double delay_ms, Functor&& f) {
	boost::posix_time::microseconds delay((int64_t)(delay_ms * 1000));
	return runEvery(delay, std::move(f));
}

DealTimerPtr EventLoop::runAfter(const boost::posix_time::time_duration& delay, Functor&& f) {
	DealTimerPtr timer(new deadline_timer(*io_service_));
	timer->expires_from_now(delay);
	timer->async_wait([timer, f](const boost::system::error_code&){ f(); });
	return timer;
}

DealTimerPtr EventLoop::runEvery(const boost::posix_time::time_duration& delay, Functor&& f) {
	std::function<void(DealTimerPtr, boost::posix_time::time_duration, Functor)> callback;
	callback = [=](DealTimerPtr timer, boost::posix_time::time_duration delay, Functor f) {
		f();
		timer->expires_from_now(delay);
		timer->async_wait(std::bind(callback, timer, std::move(delay), std::move(f)));
	};
	DealTimerPtr timer(new deadline_timer(*io_service_));	
	timer->expires_from_now(delay);
	timer->async_wait(std::bind(callback, timer, std::move(delay), std::move(f)));
	return timer;
}

void EventLoop::runInLoop(Functor&& handler) {
	io_service_->dispatch(handler);
}

void EventLoop::queueInLoop(Functor&& handler) {
	io_service_->post(handler);
}

void EventLoop::execTimerEvery(DealTimerPtr timer, boost::posix_time::time_duration delay, Functor f) {
	f();
	timer->expires_from_now(delay);
	timer->async_wait(std::bind(&EventLoop::execTimerEvery, this, timer, std::move(delay), std::move(f)));
}
}