#include <netpp/net/protobuf/codec.h>
#include <netpp/base/logging.h>
#include <netpp/net/protobuf/buffer_input_stream.h>
#include <netpp/net/protobuf/buffer_output_stream.h>

namespace netpp {
const std::string kNoErrorStr = "NoError";
const std::string kInvalidLengthStr = "InvalidLength";
const std::string kCheckSumErrorStr = "CheckSumError";
const std::string kInvalidNameLenStr = "InvalidNameLen";
const std::string kUnknownMessageTypeStr = "UnknownMessageType";
const std::string kParseErrorStr = "ParseError";
const std::string kUnknownErrorStr = "UnknownError";

int32_t asInt32(const char* buf)
{
	int32_t be32 = 0;
	::memcpy(&be32, buf, sizeof(be32));
	return htons(be32);
}

void ProtobufCodec::onMessage(const TCPConnPtr& conn, Buffer* buf) {
	while (buf->length() >= kMinMessageLen + kHeaderLen)
	{
		const int32_t len = buf->peekInt32();
		if (len > kMaxMessageLen || len < kMinMessageLen)
		{
			errorCallback_(conn, buf, kInvalidLength);
			break;
		}
		else if (buf->length() >= len + kHeaderLen)
		{
			ErrorCode errorCode = kNoError;
			buf->skip(kHeaderLen);
			MessagePtr message = parse(buf, len, &errorCode);
			if (errorCode == kNoError && message)
			{
				messageCallback_(conn, message);
			}
			else
			{
				errorCallback_(conn, buf, errorCode);
				break;
			}
		}
		else
		{
			break;
		}
	}
}

void ProtobufCodec::send(const TCPConnPtr& conn, const google::protobuf::Message& message) {
	Buffer buf;
	fillEmptyBuffer(&buf, message);
	conn->send(&buf);
}

const std::string& ProtobufCodec::errorCodeToString(ErrorCode errorCode){
	switch (errorCode)
	{
	case kNoError:
		return kNoErrorStr;
	case kInvalidLength:
		return kInvalidLengthStr;
	case kInvalidNameLen:
		return kInvalidNameLenStr;
	case kUnknownMessageType:
		return kUnknownMessageTypeStr;
	case kParseError:
		return kParseErrorStr;
	default:
		return kUnknownErrorStr;
	}
}

void ProtobufCodec::fillEmptyBuffer(Buffer* buf, const google::protobuf::Message& message) {
	const std::string& typeName = message.GetTypeName();
	int32_t nameLen = static_cast<int32_t>(typeName.size() + 1);

	buf->reservedPrepend(sizeof(kHeaderLen));
	buf->writeInt32(nameLen);
	buf->write(nameLen, typeName.c_str());
	BufferOutputStream output(buf);
	message.SerializeToZeroCopyStream(&output);
	buf->prependInt32(buf->length());
}

google::protobuf::Message* ProtobufCodec::createMessage(const std::string& type_name) {
	google::protobuf::Message* message = NULL;
	const google::protobuf::Descriptor* descriptor =
		google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if (descriptor)
	{
		const google::protobuf::Message* prototype =
			google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
		{
			message = prototype->New();
		}
	}
	return message;
}

MessagePtr ProtobufCodec::parse(Buffer* buf, int len, ErrorCode* error) {
	MessagePtr message;

	int32_t nameLen = buf->readInt32();
	if (nameLen > 0)
	{
		if (buf->length() >= nameLen){
			std::string typeName;
			buf->read(nameLen, typeName);
			message.reset(createMessage(typeName));
			if (message) {
				BufferInputStream input(buf);
				message->ParseFromZeroCopyStream(&input);
			}
			else {
				*error = kUnknownMessageType;
			}			
		}
		else {
			*error = kParseError;
		}
	}
	else
	{
		*error = kInvalidNameLen;
	}

	return message;
}

void ProtobufCodec::defaultErrorCallback(const TCPConnPtr& conn, Buffer* buf, ErrorCode errorCode) {
	LOG_ERROR << "ProtobufCodec::defaultErrorCallback - " << errorCodeToString(errorCode);
	if (conn && conn->isConnected())
	{
		conn->close();
	}
}
}