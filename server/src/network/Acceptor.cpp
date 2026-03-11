#include "network/Acceptor.h"
#include "utils/Logger.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

namespace gomoku {

Acceptor::Acceptor(EventLoop* loop, const std::string& ip, int port)
    : loop_(loop),
      acceptSocket_(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      acceptChannel_(loop, acceptSocket_.fd()),
      listening_(false),
      ip_(ip),
      port_(port) {
    
    if (acceptSocket_.fd() < 0) {
        LOG_ERROR("Acceptor::Acceptor - socket failed");
    }
    
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    
    acceptChannel_.setReadCallback([this]() { this->handleRead(); });
}

Acceptor::~Acceptor() {
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen() {
    loop_->assertInLoopThread();
    listening_ = true;
    
    if (!acceptSocket_.bindAddress(ip_, port_)) {
        LOG_ERROR("Acceptor::listen - bind failed");
        return;
    }
    
    if (!acceptSocket_.listen()) {
        LOG_ERROR("Acceptor::listen - listen failed");
        return;
    }
    
    acceptChannel_.enableReading();
    
    LOG_INFO("Acceptor listening on " + ip_ + ":" + std::to_string(port_));
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback& cb) {
    newConnectionCallback_ = cb;
}

void Acceptor::handleRead() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int connfd = ::accept(acceptSocket_.fd(), reinterpret_cast<struct sockaddr*>(&addr), &len);
    
    if (connfd >= 0) {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        int port = ntohs(addr.sin_port);
        
        LOG_INFO("New connection from " + std::string(ip) + ":" + std::to_string(port));
        
        if (newConnectionCallback_) {
            newConnectionCallback_(connfd, ip, port);
        } else {
            ::close(connfd);
        }
    } else {
        int savedErrno = errno;
        if (savedErrno == EAGAIN || savedErrno == EWOULDBLOCK) {
            // 没有新连接，正常情况
        } else {
            LOG_ERROR("Acceptor::handleRead - accept error: " + std::string(strerror(savedErrno)));
        }
    }
}

} // namespace gomoku