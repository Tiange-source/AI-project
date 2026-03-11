#include "network/EventLoop.h"
#include "network/Poller.h"
#include "network/Channel.h"
#include "utils/Logger.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

namespace gomoku {

EventLoop::EventLoop()
    : looping_(false),
      quit_(false),
      eventHandling_(false),
      threadId_(std::this_thread::get_id()),
      poller_(new Poller()),
      wakeupFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      currentActiveChannel_(nullptr),
      callingPendingFunctors_(false) {
    
    if (wakeupFd_ < 0) {
        LOG_ERROR("Failed to create eventfd");
    }
    
    wakeupChannel_->setReadCallback([this]() { this->handleRead(); });
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    quit();
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
}

void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    
    LOG_INFO("EventLoop " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " start looping");
    
    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(10000, activeChannels_);
        
        ++iteration_;
        eventHandling_ = true;
        for (Channel* channel : activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent();
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        
        doPendingFunctors();
    }
    
    LOG_INFO("EventLoop " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " stop looping");
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_) {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);
}

bool EventLoop::isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
}

void EventLoop::abortNotInLoopThread() {
    LOG_ERROR("EventLoop::abortNotInLoopThread - EventLoop was created in thread " + 
              std::to_string(reinterpret_cast<uintptr_t>(&threadId_)) + 
              ", current thread is " + 
              std::to_string(reinterpret_cast<uintptr_t>(std::this_thread::get_id())));
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    } else {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }
    
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

std::thread::id EventLoop::threadId() const {
    return threadId_;
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::wakeup() writes " + std::to_string(n) + " bytes instead of 8");
    }
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one)) {
        LOG_ERROR("EventLoop::handleRead() reads " + std::to_string(n) + " bytes instead of 8");
    }
}

void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }
    
    for (const Functor& functor : functors) {
        functor();
    }
    
    callingPendingFunctors_ = false;
}

} // namespace gomoku