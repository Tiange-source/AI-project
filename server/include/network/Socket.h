#ifndef SOCKET_H
#define SOCKET_H

#include <string>

namespace gomoku {

class Socket {
public:
    explicit Socket(int sockfd);
    ~Socket();
    
    // 禁止拷贝
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    int fd() const;
    
    // 绑定地址
    bool bindAddress(const std::string& ip, int port);
    
    // 监听
    bool listen();
    
    // 接受连接
    int accept();
    
    // 设置选项
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);
    
    // 关闭写端
    void shutdownWrite();

private:
    int sockfd_;
};

} // namespace gomoku

#endif // SOCKET_H