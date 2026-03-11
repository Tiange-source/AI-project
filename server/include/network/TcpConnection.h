#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include "network/EventLoop.h"
#include "network/Buffer.h"
#include "network/Socket.h"
#include "network/Channel.h"
#include <string>
#include <functional>
#include <memory>

namespace gomoku {

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, Buffer*)>;
    using CloseCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using WriteCompleteCallback = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    enum StateE {
        kDisconnected,
        kConnecting,
        kConnected,
        kDisconnecting
    };

    TcpConnection(EventLoop* loop, const std::string& name, int sockfd, 
                  const std::string& clientIp, int clientPort);
    ~TcpConnection();

    // 禁止拷贝
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    // 发送数据
    void send(const std::string& message);
    void send(Buffer* buf);

    // 关闭连接
    void shutdown();
    void forceClose();

    // 连接建立
    void connectEstablished();

    // 连接销毁
    void connectDestroyed();

    // 获取信息
    EventLoop* getLoop() const;
    const std::string& name() const;
    const std::string& clientIp() const;
    int clientPort() const;
    bool connected() const;

    // 设置回调
    void setConnectionCallback(const ConnectionCallback& cb);
    void setMessageCallback(const MessageCallback& cb);
    void setWriteCompleteCallback(const WriteCompleteCallback& cb);
    void setCloseCallback(const CloseCallback& cb);

    Buffer* inputBuffer();
    Buffer* outputBuffer();

private:
    enum StateE state();

    void handleRead();
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const std::string& message);
    void shutdownInLoop();
    void forceCloseInLoop();

    EventLoop* loop_;
    std::string name_;
    StateE state_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;
    std::string clientIp_;
    int clientPort_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    CloseCallback closeCallback_;
};

} // namespace gomoku

#endif // TCP_CONNECTION_H