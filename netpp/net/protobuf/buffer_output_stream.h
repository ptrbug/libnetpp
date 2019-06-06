#pragma once
#include <netpp/net/buffer.h>
#include <netpp/base/logging.h>
#include <google/protobuf/io/zero_copy_stream.h>

namespace netpp {
class BufferOutputStream : public google::protobuf::io::ZeroCopyOutputStream
{
 public:
  BufferOutputStream(Buffer* buf)
	  : buffer_(static_cast<FlatOutputBuffer*>(buf))
	  , originalSize_(buffer_->length()) {
  }

  virtual bool Next(void** data, int* size) {
	  return buffer_->flatNext(data, size);
  }

  virtual void BackUp(int count) {
	  buffer_->flatBackUp(count);
  }

  virtual google::protobuf::int64  ByteCount() const {
    return buffer_->length() - originalSize_;
  }

 private:
	 class FlatOutputBuffer : public Buffer {
	 public:
		 bool flatNext(void** data, int* size) {
			 Piece* last = back();
			 if (!last || last->off + last->len == sizeof(last->data)) {
				 push(newPiece());
				 last = back();
			 }
			 *data = last->data + last->off + last->len;
			 *size = sizeof(last->data) - last->off - last->len;
			 last->len = sizeof(last->data) - last->off;
			 length_ += *size;
			 return true;
		 }

		 void flatBackUp(size_t len) {
			 Piece* last = back();
			 assert(last);
			 assert(len < last->len);
			 last->len -= len;
			 length_ -= len;
		 }
	 };
	 FlatOutputBuffer* buffer_;
	 size_t originalSize_;
};
}
