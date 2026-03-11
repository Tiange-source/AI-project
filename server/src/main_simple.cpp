#include <iostream>
#include <csignal>
#include <memory>

// 只包含网络层头文件
#include "network/EventLoop.h"
#include "utils/Logger.h"

using namespace gomoku;

// 全局事件循环指针
EventLoop* g_loop = nullptr;

// 信号处理函数
void signalHandler(int signo) {
    if (g_loop) {
        g_loop->quit();
    }
}

int main(int argc, char* argv[]) {
    // 初始化日志
    Logger::getInstance().setLevel(LogLevel::INFO);
    LOG_INFO("Gomoku Server starting...");
    
    // 创建事件循环
    EventLoop loop;
    g_loop = &loop;
    
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    LOG_INFO("Server running, press Ctrl+C to stop");
    
    // 启动事件循环
    loop.loop();
    
    LOG_INFO("Server stopped");
    return 0;
}