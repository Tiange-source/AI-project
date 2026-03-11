#ifndef MESSAGE_DISPATCHER_H
#define MESSAGE_DISPATCHER_H

#include <google/protobuf/message.h>
#include <functional>
#include <map>
#include <string>
#include <memory>

namespace gomoku {

class TcpConnection;
using ProtobufMessagePtr = std::shared_ptr<google::protobuf::Message>;

// 消息处理回调
using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection>&, 
                                            const ProtobufMessagePtr&)>;

// 消息分发器
class MessageDispatcher {
public:
    MessageDispatcher();
    ~MessageDispatcher();
    
    // 注册消息处理回调
    template<typename T>
    void registerMessage(const std::string& typeName, 
                         const std::function<void(const std::shared_ptr<TcpConnection>&, 
                                                const std::shared_ptr<T>&)>& callback);
    
    // 分发消息
    bool dispatchMessage(const std::shared_ptr<TcpConnection>& conn, 
                         const ProtobufMessagePtr& message);
    
    // 获取消息回调
    MessageCallback getMessageCallback(const std::string& typeName) const;

private:
    std::map<std::string, MessageCallback> callbacks_;
};

// ========================================
// 模板实现
// ========================================
template<typename T>
void MessageDispatcher::registerMessage(
    const std::string& typeName, 
    const std::function<void(const std::shared_ptr<TcpConnection>&, 
                           const std::shared_ptr<T>&)>& callback) {
    
    // 包装回调，将基类指针转换为具体类型指针
    MessageCallback wrappedCallback = [callback](const std::shared_ptr<TcpConnection>& conn,
                                                 const ProtobufMessagePtr& message) {
        std::shared_ptr<T> concreteMessage = std::dynamic_pointer_cast<T>(message);
        if (concreteMessage) {
            callback(conn, concreteMessage);
        } else {
            // 类型转换失败
        }
    };
    
    callbacks_[typeName] = wrappedCallback;
}

} // namespace gomoku

#endif // MESSAGE_DISPATCHER_H