#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>

namespace gomoku {

class Channel;
class Poller;

class EventLoop {
public:
    using Functor = std::function<void()>;
    
    EventLoop();
    ~EventLoop();
    
    // 禁止拷贝
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    
    // 启动事件循环
    void loop();
    
    // 退出事件循环
    void quit();
    
    // 更新Channel
    void updateChannel(Channel* channel);
    
    // 移除Channel
    void removeChannel(Channel* channel);
    
    // 判断是否在当前线程
    bool isInLoopThread() const;
    
    // 断言在当前线程
    void assertInLoopThread();
    
    // 在I/O线程执行任务
    void runInLoop(Functor cb);
    
    // 在I/O线程排队任务
    void queueInLoop(Functor cb);
    
    // 获取线程ID
    std::thread::id threadId() const;

private:
    void abortNotInLoopThread();
    void wakeup();
    void handleRead();  // 唤醒事件回调
    void doPendingFunctors();

    std::atomic<bool> looping_;
    std::atomic<bool> quit_;
    std::atomic<bool> eventHandling_;
    const std::thread::id threadId_;
    std::unique_ptr<Poller> poller_;
    
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    
    std::vector<Channel*> activeChannels_;
    Channel* currentActiveChannel_;
    
    std::mutex mutex_;
    std::vector<Functor> pendingFunctors_;
    std::atomic<bool> callingPendingFunctors_;
    
    // 添加缺失的成员变量
    std::chrono::system_clock::time_point pollReturnTime_;
    int iteration_;
};

} // namespace gomoku

#endif // EVENT_LOOP_H