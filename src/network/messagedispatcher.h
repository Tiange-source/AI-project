#ifndef MESSAGEDISPATCHER_H
#define MESSAGEDISPATCHER_H

#include <QObject>
#include <QByteArray>
#include <functional>
#include <map>
#include <google/protobuf/message.h>

/**
 * @brief 消息处理器基类
 */
class MessageHandlerBase
{
public:
    virtual ~MessageHandlerBase() {}
    virtual void handle(const QByteArray& data) = 0;
};

/**
 * @brief 模板消息处理器
 */
template<typename MessageType>
class MessageHandler : public MessageHandlerBase
{
public:
    using HandlerFunc = std::function<void(const MessageType&)>;

    explicit MessageHandler(HandlerFunc func)
        : func_(func)
    {
    }

    void handle(const QByteArray& data) override
    {
        MessageType message;
        if (message.ParseFromArray(data.constData(), data.size())) {
            func_(message);
        } else {
            qWarning() << "Failed to parse message";
        }
    }

private:
    HandlerFunc func_;
};

/**
 * @brief 消息分发器
 *
 * 负责将接收到的消息分发到相应的处理器
 */
class MessageDispatcher : public QObject
{
    Q_OBJECT

public:
    explicit MessageDispatcher(QObject* parent = nullptr);
    ~MessageDispatcher();

    /**
     * @brief 注册消息处理器
     * @param messageType 消息类型ID
     * @param handler 处理函数
     */
    template<typename MessageType>
    void registerHandler(int messageType, std::function<void(const MessageType&)> handler)
    {
        auto messageHandler = new MessageHandler<MessageType>(handler);
        handlers_[messageType] = std::shared_ptr<MessageHandlerBase>(messageHandler);
    }

    /**
     * @brief 分发消息
     * @param messageType 消息类型ID
     * @param data 消息数据
     */
    void dispatch(int messageType, const QByteArray& data);

    /**
     * @brief 注销消息处理器
     * @param messageType 消息类型ID
     */
    void unregisterHandler(int messageType);

    /**
     * @brief 清除所有处理器
     */
    void clear();

private:
    std::map<int, std::shared_ptr<MessageHandlerBase>> handlers_;
};

#endif // MESSAGEDISPATCHER_H