#include <netpp/net/buffer.h>
#include <netpp/net/piece/piece_allocator.h>

namespace netpp {
Buffer::Buffer()
	: length_(0) {
}

Buffer::~Buffer() {
	clear();
}

void Buffer::swap(Buffer& buffer) {
	std::swap(head_, buffer.head_);
	std::swap(tail_, buffer.tail_);
	std::swap(size_, buffer.size_);
	std::swap(length_, buffer.length_);
}

size_t Buffer::length() const {
	return length_;
}

void Buffer::reservedPrepend(size_t len) {
	assert(length() == 0);
	assert(front() == nullptr);
	Piece* item = newPiece();
	assert(len < sizeof(item->data));
	item->off = len;
	item->len = 0;
	push(item);
}

void Buffer::prependInt32(int32_t x) {
	int32_t v = htonl(x);
	prepend(sizeof(v), (char*)&v);
}

void Buffer::prependInt16(int16_t x) {
	int16_t v = htons(x);
	prepend(sizeof(v), (char*)&v);
}

void Buffer::prependInt8(int8_t x) {
	prepend(sizeof(x), (char*)&x);
}

void Buffer::prepend(size_t len, const void* d) {
	Piece* first = front();
	assert(first);
	assert(len <= first->off);
	first->off -= len;
	memcpy(first->data + first->off, (char*)d, len);
	first->len += len;
	length_ += len;
}

int32_t Buffer::peekInt32() const {
	assert(length() >= sizeof(int32_t));
	int32_t v;
	peek(v);
	return ntohl(v);
}

int16_t Buffer::peekInt16() const {
	assert(length() >= sizeof(int16_t));
	int16_t v;
	peek(v);
	return ntohs(v);
}

int8_t Buffer::peekInt8() const {
	assert(length() >= sizeof(int8_t));
	int8_t v;
	peek(v);
	return v;
}

int32_t Buffer::readInt32() {
	int32_t v;
	read(sizeof(v), (char*)&v);
	return ntohl(v);
}

int16_t Buffer::readInt16() {
	int16_t v;
	read(sizeof(v), (char*)&v);
	return ntohs(v);
}

int8_t Buffer::readInt8() {
	int8_t v;
	read(sizeof(v), (char*)&v);
	return v;
}

void Buffer::read(size_t len, std::string& d) {
	assert(length() >= len);
	d.resize(len);
	read(len, (char*)d.c_str());
}

void Buffer::read(size_t len, char* d) {
	assert(len > 0);
	assert(d);
	assert(length() >= len);

	Piece* first = front();
	size_t left = len;
	do {
		if (left <= first->len){
			memcpy(d + len - left, first->data + first->off, left);
			if (left < first->len) {
				first->off += left;
				first->len -= left;
			}
			else {
				deletePiece(pop());
			}
			break;
		}
		else {
			memcpy(d + len - left, first->data + first->off, first->len);
			left -= first->len;
			deletePiece(pop());
			first = front();
		}
	} while (true);
	length_ -= len;
}

void Buffer::readAll(std::string& d) {
	read(length(), d);
}

void Buffer::read(size_t len, Buffer* buf) {
	assert(len > 0);
	assert(length() >= len);

	Piece* first = front();
	size_t left = len;
	do {
		if (left <= first->len){
			if (left < first->len) {
				Piece* buf_last = newPiece();
				memcpy(buf_last->data, first->data + first->off, left);
				buf_last->len = left;
				buf->push(buf_last);
				first->off += left;
				first->len -= left;
			}
			else {
				buf->push(pop());
			}
			break;
		}
		else {
			buf->push(pop());
			first = front();
		}
	} while (true);
	buf->length_ += len;
	length_ -= len;
}

void Buffer::writeInt32(int32_t x) {
	int32_t v = htonl(x);
	write(sizeof(v), (char*)&v);
}

void Buffer::writeInt16(int16_t x) {
	int16_t v = htons(x);
	write(sizeof(v), (char*)&v);
}

void Buffer::writeInt8(int8_t x) {
	write(sizeof(x), (char*)&x);
}

void Buffer::write(const std::string& d) {
	write(d.size(), d.c_str());
}

void Buffer::write(size_t len, const char* d) {
	Piece* last = back();
	if (!last || last->off + last->len == sizeof(last->data)){
		push(newPiece());
		last = back();
	}

	size_t left = len;
	do {
		size_t unwrite = sizeof(last->data) - last->off - last->len;
		if (left <= unwrite) {
			memcpy(last->data + last->off + last->len, d + len - left, left);
			last->len += left;
			break;
		}
		else {
			memcpy(last->data + last->off + last->len, d + len - left, unwrite);
			last->len += unwrite;
			left -= unwrite;

			push(newPiece());
			last = back();
		}
	} while (true);
	length_ += len;
}

void Buffer::skip(size_t len) {
	assert(length() >= len);
	Piece* first = front();
	size_t left = len;
	do {
		if (left <= first->len){
			if (left < first->len) {
				first->off += left;
				first->len -= left;
			}
			else {
				deletePiece(pop());
			}
			break;
		}
		else {
			left -= first->len;
			deletePiece(pop());
			first = front();
		}
	} while (true);
	length_ -= len;
}

Piece* Buffer::moveToQueueHead() {
	Piece* queue_head = head_;

	head_ = nullptr;
	tail_ = nullptr;
	size_ = 0;
	length_ = 0;

	return queue_head;
}

void Buffer::clear() {
	while (!empty()) {
		delete pop();
	}
	length_ = 0;
}
}