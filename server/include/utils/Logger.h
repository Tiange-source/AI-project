#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <sstream>

namespace gomoku {

// 日志级别
enum class LogLevel {
    DEBUG,
    INFO,
    WARN,
    ERROR
};

// 日志类
class Logger {
public:
    static Logger& getInstance();
    
    // 设置日志级别
    void setLevel(LogLevel level);
    
    // 设置日志文件路径
    void setLogFile(const std::string& filePath);
    
    // 记录日志
    void log(LogLevel level, const std::string& message);
    
    // 便捷方法
    void debug(const std::string& message);
    void info(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

private:
    Logger();
    ~Logger();
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 获取当前时间字符串
    std::string getCurrentTime();
    
    // 获取日志级别名称
    std::string getLevelName(LogLevel level);
    
    // 写入日志文件
    void writeToFile(const std::string& log);
    
    // 检查文件大小，必要时轮转
    void checkRotation();

    LogLevel level_;
    std::string logFilePath_;
    std::ofstream logFile_;
    std::mutex mutex_;
    size_t maxFileSize_;
};

// 日志流宏
#define LOG_DEBUG(msg) Logger::getInstance().debug(msg)
#define LOG_INFO(msg) Logger::getInstance().info(msg)
#define LOG_WARN(msg) Logger::getInstance().warn(msg)
#define LOG_ERROR(msg) Logger::getInstance().error(msg)

// 日志流式输出
class LogStream {
public:
    LogStream(LogLevel level);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& val) {
        oss_ << val;
        return *this;
    }

private:
    LogLevel level_;
    std::ostringstream oss_;
};

#define LOG(level) LogStream(LogLevel::level)
#define DEBUG() LOG(DEBUG)
#define INFO() LOG(INFO)
#define WARN() LOG(WARN)
#define ERROR() LOG(ERROR)

} // namespace gomoku

#endif // LOGGER_H