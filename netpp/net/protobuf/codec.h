#pragma once

#include <netpp/net/buffer.h>
#include <netpp/net/tcp_conn.h>
#include <functional>
#include <boost/noncopyable.hpp>
#include <google/protobuf/message.h>

namespace netpp {

typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class ProtobufCodec : boost::noncopyable
{
public:
	enum ErrorCode
	{
		kNoError = 0,
		kInvalidLength,
		kInvalidNameLen,
		kUnknownMessageType,
		kParseError,
	};

	typedef std::function<void(const TCPConnPtr&, const MessagePtr&)> ProtobufMessageCallback;
	typedef std::function<void(const TCPConnPtr&, Buffer*, ErrorCode)> ErrorCallback;

	explicit ProtobufCodec(const ProtobufMessageCallback& messageCb)
		: messageCallback_(messageCb)
		, errorCallback_(defaultErrorCallback) {
	}

	ProtobufCodec(const ProtobufMessageCallback& messageCb, const ErrorCallback& errorCb)
		: messageCallback_(messageCb)
		, errorCallback_(errorCb) {
	}

	void onMessage(const TCPConnPtr& conn, Buffer* buf);
	void send(const TCPConnPtr& conn, const google::protobuf::Message& message);


	static const std::string& errorCodeToString(ErrorCode errorCode);
	static void fillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message);
	static google::protobuf::Message* createMessage(const std::string& type_name);
	static MessagePtr parse(Buffer* buf, int len, ErrorCode* errorCode);

private:
	static void defaultErrorCallback(const TCPConnPtr&, Buffer*, ErrorCode);

	ProtobufMessageCallback messageCallback_;
	ErrorCallback errorCallback_;

	const static int kHeaderLen = sizeof(int32_t);
	const static int kMinMessageLen = 2 * kHeaderLen + 2;
	const static int kMaxMessageLen = 64 * 1024 * 1024;
};
}
