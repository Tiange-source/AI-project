#include "server/GameServer.h"
#include "utils/Logger.h"
#include <iostream>
#include <memory>
#include <csignal>

using namespace gomoku;

std::unique_ptr<GameServer> g_server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Received signal " + std::to_string(signal) + ", shutting down...");
        if (g_server) {
            g_server->stop();
        }
    }
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -c, --config <file>   Configuration file (default: config/server.conf)" << std::endl;
    std::cout << "  -h, --help            Print this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    // 解析命令行参数
    std::string configFile = "config/server.conf";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-c" || arg == "--config") {
            if (i + 1 < argc) {
                configFile = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file argument" << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // 注册信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // 初始化日志系统
    Logger::initialize("logs/server.log", Logger::LogLevel::INFO);
    
    LOG_INFO("========================================");
    LOG_INFO("  Gomoku Game Server");
    LOG_INFO("========================================");
    
    // 创建服务器
    g_server = std::make_unique<GameServer>();
    
    // 加载配置
    if (!g_server->loadConfig(configFile)) {
        LOG_ERROR("Failed to load config file: " + configFile);
        return 1;
    }
    
    // 初始化服务器
    if (!g_server->initialize()) {
        LOG_ERROR("Failed to initialize server");
        return 1;
    }
    
    // 启动服务器
    if (!g_server->start()) {
        LOG_ERROR("Failed to start server");
        return 1;
    }
    
    LOG_INFO("Server shutdown successfully");
    
    return 0;
}