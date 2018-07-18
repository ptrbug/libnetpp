#pragma once
#include <netpp/net/piece/piece_allocator.h>
#include <netpp/net/piece/queue.h>
#include <mutex>

namespace netpp {
class PiecePool
{
public:
	PiecePool()
		: using_count_(0) {
	}

	~PiecePool() {
		clear();
	}

	Piece* newPiece() {
		{
			std::unique_lock<std::mutex> lock(mutex_);
			using_count_++;

			Piece* item = queue_.pop();
			if (item) {
				item->next = nullptr;
				item->misalgin = 0;
				item->off = 0;
				return item;
			}
		}
		
		Piece* item = new Piece;
		item->next = nullptr;
		item->misalgin = 0;
		item->off = 0;
		return item;
	}
	
	void deletePiece(Piece* item) {
#define MAX_DELETE_ONCE 8
#define BASE_CACHE_COUNT 8

		Piece* del_array[MAX_DELETE_ONCE];
		uint32_t del_count = 0;
		
		{
			std::unique_lock<std::mutex> lock(mutex_);
			using_count_--;
			
			uint32_t limit = using_count_ + BASE_CACHE_COUNT;
			if (queue_.size() < limit) {
				queue_.push(item);
			}
			else {				
				del_count = queue_.size() - limit + 1;
				if (del_count > MAX_DELETE_ONCE) {
					del_count = MAX_DELETE_ONCE;
				}
				del_array[0] = item;
				for (uint32_t i = 1; i < del_count; ++i){
					del_array[i] = queue_.pop();
				}
			}
		}

		if (del_count > 0) {
			for (uint32_t i = 0; i < del_count; ++i) {
				delete del_array[i];
			}
		}
	}
	
private:
	void clear()
	{
		std::unique_lock<std::mutex> lock(mutex_);

		while (!queue_.empty()) {
			delete queue_.pop();
		}
	}
	
	std::mutex mutex_;
	Queue<Piece> queue_;
	uint32_t using_count_;
};

PiecePool pool_;

Piece* newPiece() {
	return pool_.newPiece();
}

void deletePiece(Piece* item) {
	pool_.deletePiece(item);
}
}