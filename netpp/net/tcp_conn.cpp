#include <netpp/net/tcp_conn.h>
#include <netpp/net/piece/piece_allocator.h>

namespace netpp {
void TCPConn::InputBuffer::writePiece(Piece* item) {
	assert(item->len > 0);
	Piece* last = back();
	if (last && last->len + item->len <= sizeof(last->data)) {
		if (last->off > 0) {
			memmove(last->data, last->data + last->off, last->len);
			last->off = 0;
		}
		memcpy(last->data + last->len, item->data + item->off, item->len);
		last->len += item->len;
		deletePiece(item);
	}
	else {
		push(item);
	}
	length_ += item->len;
}

Piece* TCPConn::OutputBuffer::readPiece() {
	assert(length() > 0);
	Piece* first = pop();
	if (first) {
		uint16_t capacity = sizeof(first->data);

		while (first->len < capacity) {
			Piece* next = front();
			if (next && first->len + next->len <= capacity) {
				if (first->off > 0) {
					memmove(first->data, first->data + first->off, first->len);
					first->off = 0;
				}
				pop();
				memcpy(first->data + first->len, next->data + next->off, next->len);
				first->len += next->len;
			}
			else {
				break;
			}
		}
		length_ -= first->len;
	}
	return first;
}

void TCPConn::OutputBuffer::writePieceQueue(Piece* queue_head){
	assert(queue_head);
	Piece* item = queue_head;
	while (item) {
		assert(item->len > 0);
		push(item);
		length_ += item->len;
		item = item->next;
	}
}

TCPConn::TCPConn(EventLoop* loop
	, SocketPtr sock
	, uint64_t id)
	: loop_(loop)
	, id_(id)
	, socket_(sock)
	, status_(kDisconnected)
	, is_sending_(false) {

}

TCPConn::~TCPConn() {
	assert(status_ == kDisconnected);
	assert(is_sending_ == false);
}

void TCPConn::send(const char* s) {
	send(s, strlen(s));
}
void TCPConn::send(const std::string& d) {
	send(d.c_str(), d.size());
}

void TCPConn::send(const void* data, size_t len) {
	if (status_ != kConnected || len == 0) {
		return;
	}

	Buffer buffer;
	buffer.write(len, (const char*)data);
	Piece* queue_head = buffer.moveToQueueHead();
	if (loop_->isInLoopThread()) {
		sendInLoop(queue_head);
	}
	else {
		loop_->runInLoop(std::bind(&TCPConn::sendInLoop, shared_from_this(), queue_head));
	}	
}

void TCPConn::send(Buffer* buffer) {
	if (status_ != kConnected || buffer->length() == 0) {
		return;
	}

	Piece* queue_head =	buffer->moveToQueueHead();
	if (loop_->isInLoopThread()) {
		sendInLoop(queue_head);
	}
	else {
		loop_->runInLoop(std::bind(&TCPConn::sendInLoop, shared_from_this(), queue_head));
	}	
}

void TCPConn::close() {
	StateE expected = kConnected;
	if (status_.compare_exchange_strong(expected, kDisconnecting)) {
		auto guardThis = shared_from_this();
		auto f = [guardThis]() {
			assert(guardThis->loop_->isInLoopThread());
			guardThis->handleClose();
		};

		loop_->queueInLoop(f);
	}
}

void TCPConn::closeWithDelay(long seconds) {
	StateE expected = kConnected;
	if (status_.compare_exchange_strong(expected, kDisconnecting)) {
		auto guardThis = shared_from_this();
		auto f = [guardThis]() {
			assert(guardThis->loop_->isInLoopThread());
			guardThis->handleClose();
		};

		delay_close_timer_ = loop_->runAfter(boost::posix_time::seconds(seconds), f);
	}	
}

void TCPConn::setTcpNoDelay(bool on) {
	socket_->set_option(boost::asio::ip::tcp::no_delay(on));
}

void TCPConn::connectEstablished() {
	assert(loop_->isInLoopThread());
	status_ = kConnected;
	local_addr_ = socket_->local_endpoint();
	remote_addr_ = socket_->remote_endpoint();
	connection_cb_(shared_from_this());
	launchRead();
}

void TCPConn::sendInLoop(Piece* queue_head) {
	output_buffer_.writePieceQueue(queue_head);
	if (!is_sending_) {
		is_sending_ = true;
		Piece* piece = output_buffer_.readPiece();
		socket_->async_send(boost::asio::buffer(piece->data + piece->off, piece->len),
			std::bind(&TCPConn::handleWrite, this, piece, std::placeholders::_1));
	}
}

void TCPConn::launchWrite() {
	if (output_buffer_.length() > 0) {
		is_sending_ = true;
		Piece* piece = output_buffer_.readPiece();
		socket_->async_send(boost::asio::buffer(piece->data + piece->off, piece->len),
			std::bind(&TCPConn::handleWrite, shared_from_this(), piece, std::placeholders::_1));
	}
}

void TCPConn::handleWrite(Piece* piece, const boost::system::error_code &ec) {
	is_sending_ = false;
	deletePiece(piece);
	if (!ec) {
		launchWrite();
	}
	else if (ec != boost::asio::error::operation_aborted) {
		handleError();
	}
}

void TCPConn::launchRead() {
	Piece* piece = newPiece();
	socket_->async_read_some(boost::asio::buffer(piece->data + piece->off, sizeof(piece->data) - piece->off),
		std::bind(&TCPConn::handleRead, shared_from_this(), piece, std::placeholders::_1, std::placeholders::_2));
}

void TCPConn::handleRead(Piece* piece, const boost::system::error_code &ec, size_t bytes_transferred) {
	if (!ec) {
		piece->len = bytes_transferred;
		input_buffer_.writePiece(piece);
		message_cb_(shared_from_this(), &input_buffer_);		
		launchRead();
	}
	else {
		deletePiece(piece);
		if (ec != boost::asio::error::operation_aborted) {
			handleError();
		}
	}
}

void TCPConn::handleError() {
	if (status_ != kDisconnected){ 
		status_ = kDisconnecting;
		handleClose();
	}
}

void TCPConn::handleClose() {
	assert(status_ == kDisconnecting);
	if (status_ != kDisconnected) {

		if (delay_close_timer_) {
			delay_close_timer_->cancel();
			delay_close_timer_.reset();
		}

		if (socket_) {
			assert(socket_->is_open());
			boost::system::error_code ec;
			socket_->shutdown(socket_base::shutdown_both, ec);			
			socket_->close(ec);
			socket_.reset();
		}		

		TCPConnPtr conn(shared_from_this());
		status_ = kDisconnecting;
		connection_cb_(conn);
		close_cb_(conn);

		status_ = kDisconnected;
	}
}
}
