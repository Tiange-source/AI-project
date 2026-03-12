#include "windowssocket.h"
#include <QDebug>
#include <QMutexLocker>
#include <WinSock2.h>
#include <ws2tcpip.h>

WindowsSocket::WindowsSocket(QObject* parent)
    : QObject(parent)
    , socket_(INVALID_SOCKET)
    , codec_(nullptr)
    , dispatcher_(nullptr)
    , connected_(false)
    , serverPort_(0)
    , receiverThread_(nullptr)
    , shouldStop_(false)
{
    // 初始化Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        qWarning() << "WSAStartup failed:" << result;
        return;
    }

    // 创建编解码器
    codec_ = new ProtobufCodec(this);

    qDebug() << "Windows socket initialized";
}

WindowsSocket::~WindowsSocket()
{
    disconnectFromServer();

    // 清理Winsock
    WSACleanup();

    qDebug() << "Windows socket destroyed";
}

bool WindowsSocket::connectToServer(const QString& host, quint16 port)
{
    if (connected_) {
        qWarning() << "Already connected to server";
        return false;
    }

    // 创建socket
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        QString error = QString("Failed to create socket: %1").arg(WSAGetLastError());
        qWarning() << error;
        emit connectionError(error);
        return false;
    }

    // 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // 转换IP地址
    #ifdef _WIN32
    // Windows使用inet_addr或InetPton
    unsigned long addr = inet_addr(host.toLocal8Bit().constData());
    if (addr == INADDR_NONE) {
        // 尝试DNS解析
        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        int status = getaddrinfo(host.toLocal8Bit().constData(), nullptr, &hints, &result);
        if (status != 0) {
            QString error = QString("Failed to resolve hostname: %1").arg(status);
            qWarning() << error;
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            emit connectionError(error);
            return false;
        }

        if (result) {
            serverAddr.sin_addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
            freeaddrinfo(result);
        } else {
            QString error = "No address found for hostname";
            qWarning() << error;
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            emit connectionError(error);
            return false;
        }
    } else {
        serverAddr.sin_addr.s_addr = addr;
    }
    #else
    // Linux/Unix使用inet_pton
    if (inet_pton(AF_INET, host.toLocal8Bit().constData(), &serverAddr.sin_addr) <= 0) {
        // 尝试DNS解析
        addrinfo hints = {};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        addrinfo* result = nullptr;
        int status = getaddrinfo(host.toLocal8Bit().constData(), nullptr, &hints, &result);
        if (status != 0) {
            QString error = QString("Failed to resolve hostname: %1").arg(status);
            qWarning() << error;
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            emit connectionError(error);
            return false;
        }

        if (result) {
            serverAddr.sin_addr = ((sockaddr_in*)result->ai_addr)->sin_addr;
            freeaddrinfo(result);
        } else {
            QString error = "No address found for hostname";
            qWarning() << error;
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
            emit connectionError(error);
            return false;
        }
    }
    #endif

    // 连接到服务器
    qDebug() << "Connecting to server:" << host << ":" << port;
    int result = ::connect(socket_, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        QString error = QString("Failed to connect to server: %1").arg(WSAGetLastError());
        qWarning() << error;
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        emit connectionError(error);
        return false;
    }

    // 设置为非阻塞模式
    u_long mode = 1;
    ioctlsocket(socket_, FIONBIO, &mode);

    // 连接成功
    connected_ = true;
    serverHost_ = host;
    serverPort_ = port;
    shouldStop_ = false;

    qDebug() << "Connected to server:" << host << ":" << port;
    emit connected();

    // 启动接收线程
    receiverThread_ = new ReceiverThread(socket_, codec_, dispatcher_, this);
    receiverThread_->start();

    return true;
}

void WindowsSocket::disconnectFromServer()
{
    if (!connected_) {
        return;
    }

    qDebug() << "Disconnecting from server";

    // 设置停止标志
    shouldStop_ = true;

    // 请求停止接收线程
    if (receiverThread_) {
        receiverThread_->requestStop();
    }

    // 关闭socket以唤醒接收线程
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }

    // 等待接收线程结束
    if (receiverThread_) {
        receiverThread_->wait();
        receiverThread_->deleteLater();
        receiverThread_ = nullptr;
    }

    connected_ = false;
    qDebug() << "Disconnected from server";
    emit disconnected();
}

bool WindowsSocket::sendMessage(const google::protobuf::Message& message, int messageType)
{
    if (!connected_) {
        qWarning() << "Not connected to server";
        return false;
    }

    if (!codec_) {
        qWarning() << "Codec is not initialized";
        return false;
    }

    // 编码消息
    QByteArray data = codec_->encode(message, messageType);
    if (data.isEmpty()) {
        qWarning() << "Failed to encode message";
        return false;
    }

    // 发送数据
    int bytesSent = send(socket_, data.constData(), data.size(), 0);
    if (bytesSent != data.size()) {
        qWarning() << "Failed to send all data:" << bytesSent << "/" << data.size();
        return false;
    }

    qDebug() << "Sent message, type:" << messageType << "size:" << bytesSent;
    return true;
}

void WindowsSocket::setMessageDispatcher(MessageDispatcher* dispatcher)
{
    dispatcher_ = dispatcher;
}

bool WindowsSocket::isConnected() const
{
    return connected_;
}

QString WindowsSocket::getServerAddress() const
{
    return serverHost_;
}

quint16 WindowsSocket::getServerPort() const
{
    return serverPort_;
}

// ReceiverThread implementation
ReceiverThread::ReceiverThread(SOCKET socket, ProtobufCodec* codec, 
                               MessageDispatcher* dispatcher, QObject* parent)
    : QThread(parent)
    , socket_(socket)
    , codec_(codec)
    , dispatcher_(dispatcher)
    , shouldStop_(false)
{
}

void ReceiverThread::requestStop()
{
    shouldStop_ = true;
}

void ReceiverThread::run()
{
    qDebug() << "Receiver thread started";

    QByteArray buffer;

    while (!shouldStop_) {
        // 接收数据
        char tempBuffer[4096];
        int bytesReceived = recv(socket_, tempBuffer, sizeof(tempBuffer), 0);

        if (bytesReceived > 0) {
            // 收到数据
            buffer.append(tempBuffer, bytesReceived);
            qDebug() << "Received" << bytesReceived << "bytes";

            // 解码消息
            while (true) {
                if (!codec_) {
                    qWarning() << "Codec is not initialized";
                    break;
                }

                int consumed = 0;
                auto result = codec_->decode(buffer, consumed);

                if (consumed <= 0) {
                    // 数据不完整，等待更多数据
                    break;
                }

                // 移除已处理的数据
                buffer = buffer.mid(consumed);

                // 分发消息
                if (dispatcher_) {
                    dispatcher_->dispatch(result.first, result.second);
                }

                qDebug() << "Processed message, type:" << result.first << "size:" << result.second.size();
            }
        } else if (bytesReceived == 0) {
            // 连接关闭
            qWarning() << "Connection closed by server";
            break;
        } else {
            // 错误
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                qWarning() << "Receive error:" << error;
                break;
            }
        }

        // 短暂休眠避免CPU占用过高
        msleep(10);
    }

    qDebug() << "Receiver thread stopped";
}