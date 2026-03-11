#ifndef BUFFER_H
#define BUFFER_H

#include <vector>
#include <string>
#include <algorithm>
#include <cstring>

namespace gomoku {

class Buffer {
public:
    static const size_t kInitialSize = 1024;
    static const size_t kCheapPrepend = 8;
    
    explicit Buffer(size_t initialSize = kInitialSize);
    
    // 获取可读数据
    const char* peek() const;
    
    // 获取可读数据长度
    size_t readableBytes() const;
    
    // 获取可写数据长度
    size_t writableBytes() const;
    
    // 获取前置空间长度
    size_t prependableBytes() const;
    
    // 已读数据
    void retrieve(size_t len);
    void retrieveUntil(const char* end);
    void retrieveAll();
    
    // 获取所有可读数据并清空
    std::string retrieveAllAsString();
    
    // 追加数据
    void append(const char* data, size_t len);
    void append(const std::string& str);
    
    // 确保可写空间
    void ensureWritableBytes(size_t len);
    
    // 获取可写空间指针
    char* beginWrite();
    const char* beginWrite() const;
    
    // 已写数据
    void hasWritten(size_t len);
    
    // 前置数据
    void prepend(const void* data, size_t len);
    
    // 缩缓冲区
    void shrink(size_t reserve);
    
    // 读取数据到缓冲区
    ssize_t readFd(int fd, int* savedErrno);
    
    // 获取数据指针
    const char* data() const;
    char* data();

private:
    // 获取缓冲区起始指针
    char* begin();
    const char* begin() const;
    
    // 扩容
    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};

} // namespace gomoku

#endif // BUFFER_H