#ifndef PROTOBUFCODEC_H
#define PROTOBUFCODEC_H

#include <QObject>
#include <QByteArray>
#include <google/protobuf/message.h>
#include <utility>

/**
 * @brief Protobuf编解码器
 *
 * 负责Protobuf消息的序列化和反序列化
 * 处理粘包和半包问题
 */
class ProtobufCodec : public QObject
{
    Q_OBJECT

public:
    explicit ProtobufCodec(QObject* parent = nullptr);
    ~ProtobufCodec();

    /**
     * @brief 消息头长度（4字节）
     */
    static const int HEADER_LENGTH = 4;

    /**
     * @brief 消息类型长度（4字节）
     */
    static const int TYPE_LENGTH = 4;

    /**
     * @brief 总消息头长度
     */
    static const int TOTAL_HEADER_LENGTH = HEADER_LENGTH + TYPE_LENGTH;

    /**
     * @brief 编码Protobuf消息
     * @param message Protobuf消息对象
     * @param messageType 消息类型ID
     * @return 编码后的字节数组
     */
    QByteArray encode(const google::protobuf::Message& message, int messageType);

    /**
     * @brief 解码字节数组
     * @param data 输入数据
     * @param consumed 消耗的字节数（输出参数）
     * @return 解码结果（消息类型，消息体）
     */
    std::pair<int, QByteArray> decode(const QByteArray& data, int& consumed);

private:
    /**
     * @brief 解析消息长度（4字节，网络字节序）
     * @param data 数据
     * @return 消息长度
     */
    int parseLength(const QByteArray& data);

    /**
     * @brief 解析消息类型（4字节，网络字节序）
     * @param data 数据
     * @return 消息类型
     */
    int parseMessageType(const QByteArray& data);

    /**
     * @brief 将整数转换为4字节网络字节序
     * @param value 整数值
     * @return 4字节数组
     */
    QByteArray int32ToBytes(int32_t value);

    /**
     * @brief 将4字节网络字节序转换为整数
     * @param bytes 4字节数组
     * @return 整数值
     */
    int32_t bytesToInt32(const QByteArray& bytes);
};

#endif // PROTOBUFCODEC_H