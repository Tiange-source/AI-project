#include "network/TcpConnection.h"
#include "utils/Logger.h"
#include <errno.h>
#include <unistd.h>

namespace gomoku {

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                             const std::string& clientIp, int clientPort)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      clientIp_(clientIp),
      clientPort_(clientPort) {

    channel_->setReadCallback([this]() { this->handleRead(); });
    channel_->setWriteCallback([this]() { this->handleWrite(); });
    channel_->setCloseCallback([this]() { this->handleClose(); });
    channel_->setErrorCallback([this]() { this->handleError(); });

    LOG_INFO("TcpConnection::TcpConnection[" + name_ + "] at " + 
             std::to_string(reinterpret_cast<uintptr_t>(this)) + 
             " fd = " + std::to_string(sockfd));
}

TcpConnection::~TcpConnection() {
    LOG_INFO("TcpConnection::~TcpConnection[" + name_ + "] at " + 
             std::to_string(reinterpret_cast<uintptr_t>(this)) + 
             " fd = " + std::to_string(channel_->fd()) + 
             " state = " + std::to_string(state_));
}

void TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    if (connectionCallback_) {
        connectionCallback_(shared_from_this());
    }
}

void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();
        if (connectionCallback_) {
            connectionCallback_(shared_from_this());
        }
    }
    channel_->remove();
}

void TcpConnection::handleRead() {
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    
    if (n > 0) {
        if (messageCallback_) {
            messageCallback_(shared_from_this(), &inputBuffer_);
        }
    } else if (n == 0) {
        handleClose();
    } else {
        errno = savedErrno;
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            LOG_ERROR("TcpConnection::handleRead error");
            handleClose();
        }
    }
}

void TcpConnection::handleWrite() {
    if (channel_->isWriting()) {
        ssize_t n = ::write(channel_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytes());
        
        if (n > 0) {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0) {
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop([this, conn = shared_from_this()]() {
                        writeCompleteCallback_(conn);
                    });
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite error");
        }
    } else {
        LOG_INFO("Connection fd = " + std::to_string(channel_->fd()) + " is down, no more writing");
    }
}

void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_INFO("TcpConnection::handleClose fd = " + std::to_string(channel_->fd()) + " state = " + std::to_string(state_));
    
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channel_->disableAll();
    
    TcpConnectionPtr guardThis(shared_from_this());
    if (connectionCallback_) {
        connectionCallback_(guardThis);
    }
    if (closeCallback_) {
        closeCallback_(guardThis);
    }
}

void TcpConnection::handleError() {
    int err = 0;
    socklen_t optlen = static_cast<socklen_t>(sizeof(err));
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &err, &optlen) < 0) {
        err = errno;
    } else {
        err = errno;
    }
    LOG_ERROR("TcpConnection::handleError [" + name_ + "] - SO_ERROR = " + std::string(strerror(err)));
}

void TcpConnection::send(const std::string& message) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(message);
        } else {
            loop_->runInLoop([this, message]() { 
                sendInLoop(message); 
            });
        }
    }
}

void TcpConnection::send(Buffer* buf) {
    if (state_ == kConnected) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->retrieveAllAsString());
        } else {
            loop_->runInLoop([this, buf]() { 
                sendInLoop(buf->retrieveAllAsString()); 
            });
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = message.size();
    bool faultError = false;

    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        
        if (nwrote >= 0) {
            remaining -= nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop([this, conn = shared_from_this()]() {
                    writeCompleteCallback_(conn);
                });
            }
        } else {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop error");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faultError = true;
                }
            }
        }
    }

    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= outputBuffer_.writableBytes()) {
            outputBuffer_.ensureWritableBytes(remaining);
        }
        outputBuffer_.append(message.data() + nwrote, remaining);
        
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        loop_->runInLoop([this]() { 
            shutdownInLoop(); 
        });
    }
}

void TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->runInLoop([this]() { 
            forceCloseInLoop(); 
        });
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        handleClose();
    }
}

EventLoop* TcpConnection::getLoop() const {
    return loop_;
}

const std::string& TcpConnection::name() const {
    return name_;
}

const std::string& TcpConnection::clientIp() const {
    return clientIp_;
}

int TcpConnection::clientPort() const {
    return clientPort_;
}

bool TcpConnection::connected() const {
    return state_ == kConnected;
}

void TcpConnection::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void TcpConnection::setMessageCallback(const MessageCallback& cb) {
    messageCallback_ = cb;
}

void TcpConnection::setWriteCompleteCallback(const WriteCompleteCallback& cb) {
    writeCompleteCallback_ = cb;
}

void TcpConnection::setCloseCallback(const CloseCallback& cb) {
    closeCallback_ = cb;
}

Buffer* TcpConnection::inputBuffer() {
    return &inputBuffer_;
}

Buffer* TcpConnection::outputBuffer() {
    return &outputBuffer_;
}

TcpConnection::StateE TcpConnection::state() {
    return state_;
}

void TcpConnection::setState(StateE s) {
    state_ = s;
}

} // namespace gomoku