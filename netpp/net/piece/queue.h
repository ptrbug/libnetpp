#pragma once
#include <stdint.h>
#include <assert.h>

namespace netpp {
template<typename T>
class Queue
{
public:
	Queue()
		: head_(nullptr)
		, tail_(nullptr)
		, size_(0) {
	}
	~Queue() {
	}

	bool empty() const {
		return size_ == 0;
	}

	uint32_t size() const	{
		return size_;
	}

	T* front() const {
		return head_;
	}

	T* back() const {
		return tail_;
	}

	void push(T* item)
	{
		assert(item != nullptr);

		item->next = nullptr;
		if (tail_){
			tail_->next = item;
		}
		else {
			assert(head_ == nullptr);
			head_ = item;
		}
		tail_ = item;
		size_++;
	}

	T* pop()
	{
		Piece* item = nullptr;
		if (head_) {
			item = head_;
			head_ = head_->next;
			if (!head_){
				tail_ = nullptr;
			}
			size_--;
		}
		return item;
	}

protected:
	T* head_;
	T* tail_;
	uint32_t size_;
};
}