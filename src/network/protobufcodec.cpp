#include "protobufcodec.h"
#include <QDebug>
#include <QDataStream>

ProtobufCodec::ProtobufCodec(QObject* parent)
    : QObject(parent)
{
}

ProtobufCodec::~ProtobufCodec()
{
}

QByteArray ProtobufCodec::encode(const google::protobuf::Message& message, int messageType)
{
    // 序列化Protobuf消息
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        qWarning() << "Failed to serialize Protobuf message";
        return QByteArray();
    }

    // 创建消息体
    QByteArray body(serialized.data(), static_cast<int>(serialized.size()));

    // 创建消息头：[长度(4字节)][类型(4字节)]
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);  // 网络字节序

    // 写入消息长度（不包含消息头的长度）
    stream << static_cast<int32_t>(body.size());

    // 写入消息类型
    stream << static_cast<int32_t>(messageType);

    // 组合完整消息：[消息头][消息体]
    QByteArray fullMessage;
    fullMessage.append(header);
    fullMessage.append(body);

    return fullMessage;
}

std::pair<int, QByteArray> ProtobufCodec::decode(const QByteArray& data, int& consumed)
{
    consumed = 0;

    // 检查是否有足够的数据读取消息头
    if (data.size() < TOTAL_HEADER_LENGTH) {
        // 数据不完整，等待更多数据
        return std::make_pair(0, QByteArray());
    }

    // 解析消息长度
    int messageLength = parseLength(data);
    if (messageLength <= 0 || messageLength > 1024 * 1024) {  // 限制最大1MB
        qWarning() << "Invalid message length:" << messageLength;
        return std::make_pair(0, QByteArray());
    }

    // 解析消息类型
    int messageType = parseMessageType(data);
    if (messageType <= 0) {
        qWarning() << "Invalid message type:" << messageType;
        return std::make_pair(0, QByteArray());
    }

    // 检查是否有完整的数据
    int totalLength = TOTAL_HEADER_LENGTH + messageLength;
    if (data.size() < totalLength) {
        // 消息不完整，等待更多数据
        return std::make_pair(0, QByteArray());
    }

    // 提取消息体
    QByteArray body = data.mid(TOTAL_HEADER_LENGTH, messageLength);
    consumed = totalLength;

    return std::make_pair(messageType, body);
}

int ProtobufCodec::parseLength(const QByteArray& data)
{
    if (data.size() < HEADER_LENGTH) {
        return 0;
    }

    return bytesToInt32(data.left(HEADER_LENGTH));
}

int ProtobufCodec::parseMessageType(const QByteArray& data)
{
    if (data.size() < TOTAL_HEADER_LENGTH) {
        return 0;
    }

    return bytesToInt32(data.mid(HEADER_LENGTH, TYPE_LENGTH));
}

QByteArray ProtobufCodec::int32ToBytes(int32_t value)
{
    QByteArray bytes(4, 0);
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << value;
    return bytes;
}

int32_t ProtobufCodec::bytesToInt32(const QByteArray& bytes)
{
    if (bytes.size() != 4) {
        return 0;
    }

    int32_t value = 0;
    QDataStream stream(bytes);
    stream.setByteOrder(QDataStream::BigEndian);
    stream >> value;
    return value;
}