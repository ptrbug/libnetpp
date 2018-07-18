#pragma once
#include <netpp/net/tcp_conn.h>
#include <google/protobuf/message.h>
#include <map>
#include <functional>
#include <boost/noncopyable.hpp>

namespace netpp {
typedef std::shared_ptr<google::protobuf::Message> MessagePtr;

class Callback : boost::noncopyable
{
public:
	virtual ~Callback() {};
	virtual void onMessage(const TCPConnPtr&, const MessagePtr& message) const = 0;
};

template <typename T>
class CallbackT : public Callback
{
	BOOST_STATIC_ASSERT((boost::is_base_of<google::protobuf::Message, T>::value));

public:
	typedef std::function<void(const TCPConnPtr&, const std::shared_ptr<T>& message)> ProtobufMessageTCallback;

	CallbackT(const ProtobufMessageTCallback& callback)
		: callback_(callback) {
	}

	virtual void onMessage(const TCPConnPtr& conn, const MessagePtr& message) const {
		std::shared_ptr<T> concrete = std::static_pointer_cast<T>(message);
		assert(concrete != NULL);
		callback_(conn, concrete);
	}

private:
	ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher
{
public:
	typedef std::function<void(const TCPConnPtr&, const MessagePtr& message)> ProtobufMessageCallback;

	explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
		: defaultCallback_(defaultCb) {
	}

	void onProtobufMessage(const TCPConnPtr& conn, const MessagePtr& message) const	{
		CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
		if (it != callbacks_.end())	{
			it->second->onMessage(conn, message);
		}
		else {
			defaultCallback_(conn, message);
		}
	}

	template<typename T>
	void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback) {
		boost::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
		callbacks_[T::descriptor()] = pd;
	}

private:
	typedef std::map<const google::protobuf::Descriptor*, boost::shared_ptr<Callback> > CallbackMap;

	CallbackMap callbacks_;
	ProtobufMessageCallback defaultCallback_;
};
}