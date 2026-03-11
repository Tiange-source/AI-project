#include "utils/ThreadPool.h"
#include "utils/Logger.h"

namespace gomoku {

ThreadPool::ThreadPool(size_t threadCount) : stop_(false) {
    for (size_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&ThreadPool::workerThread, this);
    }
    
    LOG_INFO("ThreadPool initialized with " + std::to_string(threadCount) + " threads");
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::workerThread() {
    while (true) {
        Task task;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            
            condition_.wait(lock, [this] { 
                return stop_ || !tasks_.empty(); 
            });
            
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        try {
            task();
        } catch (const std::exception& e) {
            LOG_ERROR(std::string("Task exception: ") + e.what());
        }
    }
}

size_t ThreadPool::getThreadCount() const {
    return workers_.size();
}

size_t ThreadPool::getTaskCount() const {
    std::unique_lock<std::mutex> lock(queueMutex_);
    return tasks_.size();
}

void ThreadPool::shutdown() {
    if (stop_) {
        return;
    }
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
    }
    
    condition_.notify_all();
    
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    
    LOG_INFO("ThreadPool shutdown");
}

} // namespace gomoku