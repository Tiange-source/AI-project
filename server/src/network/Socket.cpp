#include "network/Socket.h"
#include "utils/Logger.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace gomoku {

Socket::Socket(int sockfd) : sockfd_(sockfd) {
}

Socket::~Socket() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
    }
}

int Socket::fd() const {
    return sockfd_;
}

bool Socket::bindAddress(const std::string& ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        LOG_ERROR("Socket::bindAddress - invalid ip address: " + ip);
        return false;
    }
    
    if (::bind(sockfd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        LOG_ERROR("Socket::bindAddress - bind failed: " + std::string(strerror(errno)));
        return false;
    }
    
    return true;
}

bool Socket::listen() {
    if (::listen(sockfd_, SOMAXCONN) < 0) {
        LOG_ERROR("Socket::listen - listen failed: " + std::string(strerror(errno)));
        return false;
    }
    return true;
}

int Socket::accept() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int connfd = ::accept(sockfd_, reinterpret_cast<struct sockaddr*>(&addr), &len);
    
    if (connfd < 0) {
        int savedErrno = errno;
        LOG_ERROR("Socket::accept - accept failed: " + std::string(strerror(savedErrno)));
    }
    
    return connfd;
}

void Socket::setReuseAddr(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReusePort(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setKeepAlive(bool on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
}

} // namespace gomoku