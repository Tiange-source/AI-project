#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "network/Socket.h"
#include "network/Channel.h"
#include "network/EventLoop.h"
#include <functional>
#include <string>

namespace gomoku {

class Acceptor {
public:
    using NewConnectionCallback = std::function<void(int sockfd, const std::string& ip, int port)>;
    
    Acceptor(EventLoop* loop, const std::string& ip, int port);
    ~Acceptor();
    
    // 禁止拷贝
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
    
    // 开始监听
    void listen();
    
    // 设置新连接回调
    void setNewConnectionCallback(const NewConnectionCallback& cb);

private:
    void handleRead();

    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
    std::string ip_;
    int port_;
};

} // namespace gomoku

#endif // ACCEPTOR_H