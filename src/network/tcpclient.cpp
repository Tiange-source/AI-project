#include "tcpclient.h"
#include "protobufcodec.h"
#include "messagedispatcher.h"
#include <QDebug>

TcpClient::TcpClient(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , codec_(nullptr)
    , dispatcher_(nullptr)
    , state_(DISCONNECTED)
    , serverPort_(0)
{
    // 连接socket信号
    connect(socket_, &QTcpSocket::connected, this, &TcpClient::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &TcpClient::onError);

    // 创建编解码器
    codec_ = new ProtobufCodec(this);
}

TcpClient::~TcpClient()
{
    if (socket_->isOpen()) {
        socket_->close();
    }
}

void TcpClient::connectToServer(const QString& host, quint16 port)
{
    if (state_ == CONNECTED) {
        qWarning() << "Already connected to server";
        return;
    }

    serverHost_ = host;
    serverPort_ = port;
    state_ = CONNECTING;
    buffer_.clear();

    qDebug() << "Connecting to server:" << host << ":" << port;

    socket_->connectToHost(host, port);
}

void TcpClient::disconnectFromServer()
{
    if (socket_->isOpen()) {
        qDebug() << "Disconnecting from server";
        socket_->disconnectFromHost();
    }
}

bool TcpClient::sendMessage(const google::protobuf::Message& message, int messageType)
{
    if (state_ != CONNECTED) {
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
    qint64 bytesWritten = socket_->write(data);
    if (bytesWritten != data.size()) {
        qWarning() << "Failed to write all data:" << bytesWritten << "/" << data.size();
        return false;
    }

    socket_->flush();
    return true;
}

void TcpClient::setMessageDispatcher(MessageDispatcher* dispatcher)
{
    dispatcher_ = dispatcher;
}

ConnectionState TcpClient::getConnectionState() const
{
    return state_;
}

bool TcpClient::isConnected() const
{
    return state_ == CONNECTED;
}

QString TcpClient::getServerAddress() const
{
    return serverHost_;
}

quint16 TcpClient::getServerPort() const
{
    return serverPort_;
}

void TcpClient::onConnected()
{
    state_ = CONNECTED;
    qDebug() << "Connected to server:" << serverHost_ << ":" << serverPort_;
    emit connected();
}

void TcpClient::onDisconnected()
{
    state_ = DISCONNECTED;
    qDebug() << "Disconnected from server";
    buffer_.clear();
    emit disconnected();
}

void TcpClient::onReadyRead()
{
    // 读取所有可用数据
    QByteArray data = socket_->readAll();
    buffer_.append(data);

    // 解码消息
    while (true) {
        if (!codec_) {
            qWarning() << "Codec is not initialized";
            break;
        }

        int consumed = 0;
        auto result = codec_->decode(buffer_, consumed);

        if (consumed <= 0) {
            // 数据不完整，等待更多数据
            break;
        }

        // 移除已处理的数据
        buffer_ = buffer_.mid(consumed);

        // 分发消息
        if (dispatcher_) {
            dispatcher_->dispatch(result.first, result.second);
        } else {
            emit messageReceived(result.first, result.second);
        }
    }
}

void TcpClient::onError(QAbstractSocket::SocketError socketError)
{
    QString errorMsg = socket_->errorString();
    qWarning() << "Socket error:" << socketError << errorMsg;

    if (socketError == QAbstractSocket::ConnectionRefusedError ||
        socketError == QAbstractSocket::RemoteHostClosedError ||
        socketError == QAbstractSocket::NetworkError ||
        socketError == QAbstractSocket::SocketTimeoutError) {
        state_ = DISCONNECTED;
    }

    emit connectionError(errorMsg);
}