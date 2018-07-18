#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>

namespace netpp {
using namespace boost::asio;

class Buffer;
class TCPConn;
typedef boost::shared_ptr<ip::tcp::socket> SocketPtr;
typedef std::shared_ptr<TCPConn> TCPConnPtr;


typedef std::shared_ptr<TCPConn> TCPConnPtr;
typedef std::function<void(const TCPConnPtr& conn)> ConnectionCallback;
typedef std::function<void(const TCPConnPtr& conn)> CloseCallback;
typedef std::function<void(const TCPConnPtr& conn)> WriteCompleteCallback;

typedef std::function<void(const TCPConnPtr& conn, Buffer* buffer)> MessageCallback;
typedef std::function<bool(const ip::tcp::endpoint& remote_addr)> VerifyAddressCallback;

namespace internal {
	inline void defaultConnectionCallback(const TCPConnPtr&) {}
	inline void defaultMessageCallback(const TCPConnPtr& conn, Buffer* buffer) {}
	inline bool defaultVerifyAddressCallback(const ip::tcp::endpoint& remote_addr) { return true; }
}
}