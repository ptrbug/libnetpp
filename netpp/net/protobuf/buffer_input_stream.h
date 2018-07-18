#pragma once
#include <netpp/net/buffer.h>
#include <netpp/net/piece/piece_allocator.h>
#include <netpp/base/logging.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace netpp {
class BufferInputStream : public google::protobuf::io::ZeroCopyInputStream
{
public:
	BufferInputStream(Buffer* buf)
		: buffer_(static_cast<FlatInputBuffer*>(buf))
		, drop_(nullptr)
		, originalSize_(buffer_->length()) {
	}

	~BufferInputStream() {
		if (drop_){
			deletePiece(drop_);
		}
	}

	virtual bool Next(const void** data, int* size) {
		if (drop_){
			deletePiece(drop_);
			drop_ = nullptr;
		}
		return buffer_->flatNext(data, size, &drop_);
	}

	virtual void BackUp(int count) {
		assert(drop_);
		buffer_->flatBackUp(count, drop_);
		drop_ = nullptr;
	}

	virtual bool Skip(int count) {
		if (count <= buffer_->length()) {
			buffer_->skip(count);
			return true;
		}
		return false;
	}

	virtual google::protobuf::int64 ByteCount() const {
		return originalSize_ - buffer_->length();
	}

private:
	class FlatInputBuffer : public Buffer{
	public:
		bool flatNext(const void** data, int* size, Piece** drop_ptr) {
			Piece* first = front();
			if (first && first->off > 0) {
				*data = first->data + first->misalgin;
				*size = first->off;
				length_ -= first->off;
				*drop_ptr = pop();
				return true;
			}
			else{
				*data = nullptr;
				*size = 0;
				return false;
			}
		}

		void flatBackUp(size_t len, Piece* drop){
			assert(drop);
			assert(drop->off > len);
			drop->misalgin += drop->off - len;
			drop->off = len;

			drop->next = head_;
			head_ = drop;
			size_++;
			length_ += len;
		}
	};
	FlatInputBuffer* buffer_;
	Piece* drop_;
	size_t originalSize_;
};
}
