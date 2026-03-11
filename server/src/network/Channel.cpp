#include "network/Channel.h"
#include "network/EventLoop.h"
#include "utils/Logger.h"
#include <sys/epoll.h>

namespace gomoku {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1) {
}

Channel::~Channel() {
    // 禁用所有事件
    disableAll();
}

void Channel::handleEvent() {
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (closeCallback_) {
            closeCallback_();
        }
    }
    
    if (revents_ & (EPOLLERR | EPOLLHUP)) {
        if (errorCallback_) {
            errorCallback_();
        }
    }
    
    if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback_) {
            readCallback_();
        }
    }
    
    if (revents_ & EPOLLOUT) {
        if (writeCallback_) {
            writeCallback_();
        }
    }
}

void Channel::setReadCallback(const EventCallback& cb) {
    readCallback_ = cb;
}

void Channel::setWriteCallback(const EventCallback& cb) {
    writeCallback_ = cb;
}

void Channel::setErrorCallback(const EventCallback& cb) {
    errorCallback_ = cb;
}

void Channel::setCloseCallback(const EventCallback& cb) {
    closeCallback_ = cb;
}

void Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

bool Channel::isReading() const {
    return events_ & kReadEvent;
}

bool Channel::isWriting() const {
    return events_ & kWriteEvent;
}

bool Channel::isNoneEvent() const {
    return events_ == kNoneEvent;
}

int Channel::fd() const {
    return fd_;
}

int Channel::events() const {
    return events_;
}

void Channel::setRevents(int revt) {
    revents_ = revt;
}

int Channel::index() const {
    return index_;
}

void Channel::setIndex(int idx) {
    index_ = idx;
}

EventLoop* Channel::ownerLoop() {
    return loop_;
}

void Channel::remove() {
    disableAll();
    loop_->removeChannel(this);
}

void Channel::update() {
    loop_->updateChannel(this);
}

} // namespace gomoku