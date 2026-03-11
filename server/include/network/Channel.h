#ifndef CHANNEL_H
#define CHANNEL_H

#include <functional>
#include <memory>

namespace gomoku {

class EventLoop;

class Channel {
public:
    using EventCallback = std::function<void()>;
    
    Channel(EventLoop* loop, int fd);
    ~Channel();
    
    // 禁止拷贝
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    
    // 处理事件
    void handleEvent();
    
    // 设置回调函数
    void setReadCallback(const EventCallback& cb);
    void setWriteCallback(const EventCallback& cb);
    void setErrorCallback(const EventCallback& cb);
    void setCloseCallback(const EventCallback& cb);
    
    // 启用/禁用事件
    void enableReading();
    void disableReading();
    void enableWriting();
    void disableWriting();
    void disableAll();
    
    // 检查事件状态
    bool isReading() const;
    bool isWriting() const;
    bool isNoneEvent() const;
    
    // 获取/设置文件描述符
    int fd() const;
    
    // 获取/设置事件
    int events() const;
    void setRevents(int revt);
    
    // 获取/设置索引
    int index() const;
    void setIndex(int idx);
    
    // 获取EventLoop
    EventLoop* ownerLoop();

private:
    void update();

    EventLoop* loop_;
    const int fd_;
    int events_;
    int revents_;
    int index_;
    
    EventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback closeCallback_;
};

} // namespace gomoku

#endif // CHANNEL_H