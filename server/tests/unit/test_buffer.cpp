#include "test_framework.h"
#include "network/Buffer.h"
#include <cstring>

using namespace gomoku;

// Buffer测试套件
TEST_TESTCASE(Buffer, DefaultConstruction) {
    Buffer buf;
    
    TEST_ASSERT_EQ(buf.readableBytes(), 0);
    TEST_ASSERT_EQ(buf.writableBytes(), Buffer::kInitialSize);
}

TEST_TESTCASE(Buffer, AppendData) {
    Buffer buf;
    const char* data = "Hello, World!";
    
    buf.append(data, strlen(data));
    
    TEST_ASSERT_EQ(buf.readableBytes(), strlen(data));
    TEST_ASSERT_EQ(buf.writableBytes(), Buffer::kInitialSize - strlen(data));
}

TEST_TESTCASE(Buffer, ReadData) {
    Buffer buf;
    const char* data = "Hello, World!";
    
    buf.append(data, strlen(data));
    
    std::string result = buf.retrieveAllAsString();
    
    TEST_ASSERT_EQ(result, data);
    TEST_ASSERT_EQ(buf.readableBytes(), 0);
}

TEST_TESTCASE(Buffer, Retrieve) {
    Buffer buf;
    const char* data = "Hello, World!";
    
    buf.append(data, strlen(data));
    
    // 读取前5个字节
    buf.retrieve(5);
    
    TEST_ASSERT_EQ(buf.readableBytes(), strlen(data) - 5);
    TEST_ASSERT_EQ(buf.peek()[0], ',');
}

TEST_TESTCASE(Buffer, RetrieveAll) {
    Buffer buf;
    const char* data = "Hello, World!";
    
    buf.append(data, strlen(data));
    
    buf.retrieveAll();
    
    TEST_ASSERT_EQ(buf.readableBytes(), 0);
}

TEST_TESTCASE(Buffer, AppendString) {
    Buffer buf;
    std::string str = "Hello, World!";
    
    buf.append(str);
    
    TEST_ASSERT_EQ(buf.readableBytes(), str.size());
}

TEST_TESTCASE(Buffer, EnsureWritableBytes) {
    Buffer buf(16);  // 初始大小16
    const char* data = "This is a long string that exceeds initial buffer size";
    
    buf.append(data, strlen(data));
    
    TEST_ASSERT_EQ(buf.readableBytes(), strlen(data));
}

TEST_TESTCASE(Buffer, BeginWrite) {
    Buffer buf;
    const char* data = "Hello";
    
    buf.append(data, strlen(data));
    
    char* writePtr = buf.beginWrite();
    TEST_ASSERT_NOT_NULL(writePtr);
}

TEST_TESTCASE(Buffer, HasWritten) {
    Buffer buf;
    
    char* writePtr = buf.beginWrite();
    strcpy(writePtr, "Hello");
    buf.hasWritten(5);
    
    TEST_ASSERT_EQ(buf.readableBytes(), 5);
}

TEST_TESTCASE(Buffer, Prepend) {
    Buffer buf;
    const char* data = "World";
    
    buf.append(data, strlen(data));
    
    // 在前面添加数据
    const char* prependData = "Hello, ";
    buf.prepend(prependData, strlen(prependData));
    
    std::string result = buf.retrieveAllAsString();
    TEST_ASSERT_EQ(result, "Hello, World");
}

TEST_TESTCASE(Buffer, Shrink) {
    Buffer buf(1024);
    const char* data = "Hello";
    
    buf.append(data, strlen(data));
    buf.retrieveAll();
    
    buf.shrink(0);
    
    // 缩小后缓冲区应该变小
    TEST_ASSERT_TRUE(buf.writableBytes() < 1024);
}