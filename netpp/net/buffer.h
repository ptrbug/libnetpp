#pragma once
#include <netpp/net/piece/queue.h>
#include <string>
#include <boost/asio.hpp>

namespace netpp{
struct Piece;
class Buffer : protected Queue<Piece>
{
public:
	explicit Buffer();
	~Buffer();

	void swap(Buffer& buffer);

	size_t length() const;

	void reservedPrepend(size_t len);
	void prependInt32(int32_t x);
	void prependInt16(int16_t x);
	void prependInt8(int8_t x);
	void prepend(size_t len, const void* d);	

	int32_t peekInt32() const;
	int16_t peekInt16() const;
	int8_t peekInt8() const;

	int32_t readInt32();
	int16_t readInt16();
	int8_t readInt8();
	void read(size_t len, std::string& d);	
	void read(size_t len, char* d);	
	void readAll(std::string& d);
	void read(size_t len, Buffer* empty_buf);

	void writeInt32(int32_t x);
	void writeInt16(int16_t x);
	void writeInt8(int8_t x);
	void write(const std::string& d);
	void write(size_t len, const char* d);	

	void skip(size_t len);

	Piece* moveToQueueHead();

	void clear();

protected:
	template<typename T>
	void peek(T& t) const {
		assert(length() >= sizeof(t));

		Piece* cur = front();
		if (sizeof(T) <= cur->off) {
			memcpy(&t, cur->data + cur->misalgin, sizeof(T));
		}
		else {
			memcpy(&t, cur->data + cur->misalgin, cur->off);			
			size_t left = sizeof(T) - cur->off;
			cur = cur->next;
			do {
				if (left <= cur->off) {
					memcpy(&t + sizeof(T) - left, cur->data + cur->misalgin, left);
					break;
				}
				else {
					memcpy(&t + sizeof(T) - left, cur->data + cur->misalgin, cur->off);
					left -= cur->off;
					cur = cur->next;
				}
			} while (true);
		}
	}

	size_t length_;
};
}