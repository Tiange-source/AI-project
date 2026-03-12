#ifndef WINDOWSSOCKET_H
#define WINDOWSSOCKET_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "protobufcodec.h"
#include "messagedispatcher.h"

#pragma comment(lib, "ws2_32.lib")

// 前向声明
class ReceiverThread;

/**
 * @brief Windows Socket网络客户端
 *
 * 使用Windows Winsock API实现TCP客户端
 */
class WindowsSocket : public QObject
{
    Q_OBJECT

public:
    explicit WindowsSocket(QObject* parent = nullptr);
    ~WindowsSocket();

    /**
     * @brief 连接到服务器
     * @param host 服务器地址
     * @param port 服务器端口
     */
    bool connectToServer(const QString& host, quint16 port);

    /**
     * @brief 断开连接
     */
    void disconnectFromServer();

    /**
     * @brief 发送消息
     * @param message Protobuf消息
     * @param messageType 消息类型
     * @return 是否成功
     */
    bool sendMessage(const google::protobuf::Message& message, int messageType);

    /**
     * @brief 设置消息分发器
     */
    void setMessageDispatcher(MessageDispatcher* dispatcher);

    /**
     * @brief 获取连接状态
     */
    bool isConnected() const;

    /**
     * @brief 获取服务器地址
     */
    QString getServerAddress() const;

    /**
     * @brief 获取服务器端口
     */
    quint16 getServerPort() const;

signals:
    /**
     * @brief 连接成功信号
     */
    void connected();

    /**
     * @brief 连接断开信号
     */
    void disconnected();

    /**
     * @brief 连接错误信号
     * @param error 错误信息
     */
    void connectionError(const QString& error);

    /**
     * @brief 收到消息信号
     * @param messageType 消息类型
     * @param data 消息数据
     */
    void messageReceived(int messageType, const QByteArray& data);

private:
    SOCKET socket_;
    ProtobufCodec* codec_;
    MessageDispatcher* dispatcher_;
    bool connected_;
    QString serverHost_;
    quint16 serverPort_;
    ReceiverThread* receiverThread_;
    bool shouldStop_;

    /**
     * @brief 接收线程
     */
    void receiverLoop();
};

/**
 * @brief 接收线程类
 */
class ReceiverThread : public QThread
{
    Q_OBJECT

public:
    explicit ReceiverThread(SOCKET socket, ProtobufCodec* codec, 
                           MessageDispatcher* dispatcher, QObject* parent = nullptr);
    
    /**
     * @brief 请求停止线程
     */
    void requestStop();

protected:
    void run() override;

private:
    SOCKET socket_;
    ProtobufCodec* codec_;
    MessageDispatcher* dispatcher_;
    std::atomic<bool> shouldStop_;
};

#endif // WINDOWSSOCKET_H