#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <functional>
#include <google/protobuf/message.h>

// 连接状态枚举
enum ConnectionState {
    DISCONNECTED = 0,
    CONNECTING = 1,
    CONNECTED = 2
};

// 前向声明
class ProtobufCodec;
class MessageDispatcher;

/**
 * @brief TCP客户端类
 *
 * 负责与服务器的TCP连接管理和消息收发
 */
class TcpClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpClient(QObject* parent = nullptr);
    ~TcpClient();

    /**
     * @brief 连接到服务器
     * @param host 服务器地址
     * @param port 服务器端口
     */
    void connectToServer(const QString& host, quint16 port);

    /**
     * @brief 断开连接
     */
    void disconnectFromServer();

    /**
     * @brief 发送Protobuf消息
     * @param message Protobuf消息对象
     * @param messageType 消息类型ID
     * @return 发送是否成功
     */
    bool sendMessage(const google::protobuf::Message& message, int messageType);

    /**
     * @brief 设置消息处理器
     * @param dispatcher 消息分发器
     */
    void setMessageDispatcher(MessageDispatcher* dispatcher);

    /**
     * @brief 获取连接状态
     * @return 连接状态
     */
    ConnectionState getConnectionState() const;

    /**
     * @brief 检查是否已连接
     * @return 是否已连接
     */
    bool isConnected() const;

    /**
     * @brief 获取服务器地址
     * @return 服务器地址
     */
    QString getServerAddress() const;

    /**
     * @brief 获取服务器端口
     * @return 服务器端口
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
     * @brief 消息接收信号
     * @param messageType 消息类型
     * @param data 消息数据
     */
    void messageReceived(int messageType, const QByteArray& data);

private slots:
    /**
     * @brief 连接建立槽函数
     */
    void onConnected();

    /**
     * @brief 连接断开槽函数
     */
    void onDisconnected();

    /**
     * @brief 有数据可读槽函数
     */
    void onReadyRead();

    /**
     * @brief 连接错误槽函数
     * @param socketError socket错误
     */
    void onError(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket* socket_;              // TCP套接字
    ProtobufCodec* codec_;            // Protobuf编解码器
    MessageDispatcher* dispatcher_;   // 消息分发器
    ConnectionState state_;           // 连接状态
    QString serverHost_;              // 服务器地址
    quint16 serverPort_;              // 服务器端口
    QByteArray buffer_;               // 接收缓冲区
};

#endif // TCPCLIENT_H