#include "utils/Logger.h"
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>

namespace gomoku {

Logger::Logger() 
    : level_(LogLevel::INFO), 
      maxFileSize_(100 * 1024 * 1024) { // 100MB
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel level) {
    level_ = level;
}

void Logger::setLogFile(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (logFile_.is_open()) {
        logFile_.close();
    }
    
    logFilePath_ = filePath;
    logFile_.open(filePath, std::ios::app);
    
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << filePath << std::endl;
    }
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::getLevelName(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void Logger::writeToFile(const std::string& log) {
    if (logFile_.is_open()) {
        logFile_ << log << std::endl;
        logFile_.flush();
    }
}

void Logger::checkRotation() {
    if (logFilePath_.empty() || !logFile_.is_open()) {
        return;
    }
    
    logFile_.flush();
    logFile_.seekp(0, std::ios::end);
    size_t fileSize = logFile_.tellp();
    
    if (fileSize >= maxFileSize_) {
        logFile_.close();
        
        // 重命名当前日志文件
        std::string oldFile = logFilePath_ + ".old";
        std::rename(logFilePath_.c_str(), oldFile.c_str());
        
        // 打开新文件
        logFile_.open(logFilePath_, std::ios::app);
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open new log file after rotation" << std::endl;
        }
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < level_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string log = "[" + getCurrentTime() + "] " +
                      "[" + getLevelName(level) + "] " +
                      message;
    
    // 输出到控制台
    std::cout << log << std::endl;
    
    // 输出到文件
    writeToFile(log);
    checkRotation();
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

// ========================================
// LogStream 实现
// ========================================
LogStream::LogStream(LogLevel level) : level_(level) {
}

LogStream::~LogStream() {
    Logger::getInstance().log(level_, oss_.str());
}

} // namespace gomoku