#ifndef PROTOBUF_CODEC_H
#define PROTOBUF_CODEC_H

#include <google/protobuf/message.h>
#include <functional>
#include <string>
#include <memory>
#include <unordered_map>

namespace gomoku {

class TcpConnection;
class Buffer;

// 前向声明protobuf消息类型
namespace gomoku {
class LoginRequest;
class LoginResponse;
class RegisterRequest;
class RegisterResponse;
}

// Protobuf编解码器
class ProtobufCodec {
public:
    using ProtobufMessagePtr = std::shared_ptr<google::protobuf::Message>;
    
    // 消息回调类型
    using ProtobufMessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, 
                                                       const ProtobufMessagePtr&)>;
    
    // 错误回调类型
    using ErrorCallback = std::function<void(const std::shared_ptr<TcpConnection>&, 
                                             Buffer*, 
                                             int, 
                                             const std::string&)>;
    
    ProtobufCodec();
    ~ProtobufCodec();
    
    // 编码消息
    std::string encode(const google::protobuf::Message& message);
    
    // 解码消息
    void decode(const std::shared_ptr<TcpConnection>& conn, Buffer* buf);
    
    // 设置消息回调
    void setMessageCallback(const ProtobufMessageCallback& cb);
    
    // 设置错误回调
    void setErrorCallback(const ErrorCallback& cb);
    
    // 根据消息类型创建消息
    static ProtobufMessagePtr createMessage(int messageType);
    
    // 注册消息类型到名称的映射
    static void registerMessageType(int messageType, const std::string& typeName);
    
    // 注册消息工厂
    template<typename T>
    static void registerMessageFactory(int messageType) {
        messageFactories_[messageType] = []() -> ProtobufMessagePtr {
            return std::make_shared<T>();
        };
    }

private:
    // 检查消息完整性
    bool checkComplete(const char* data, size_t len, size_t* messageLen);
    
    // 获取消息类型
    static int getMessageType(const google::protobuf::Message& message);
    
    // 默认错误回调
    static void defaultErrorCallback(const std::shared_ptr<TcpConnection>& conn, 
                                     Buffer* buf, 
                                     int errorCode, 
                                     const std::string& errorMessage);

    ProtobufMessageCallback messageCallback_;
    ErrorCallback errorCallback_;
    
    // 消息工厂
    static std::unordered_map<int, std::function<ProtobufMessagePtr()>> messageFactories_;
    
    // 消息类型到名称的映射
    static std::unordered_map<int, std::string> messageTypeNames_;
    
    static const int kHeaderLen = sizeof(int32_t) + sizeof(int32_t);  // [长度][类型]
    static const int kMaxMessageLen = 64 * 1024 * 1024;  // 64MB
};

} // namespace gomoku

#endif // PROTOBUF_CODEC_H