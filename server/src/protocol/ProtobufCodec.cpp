#include "protocol/ProtobufCodec.h"
#include "network/TcpConnection.h"
#include "network/Buffer.h"
#include "utils/Logger.h"
#include <arpa/inet.h>
#include <cstring>

namespace gomoku {

const int ProtobufCodec::kHeaderLen;
const int ProtobufCodec::kMaxMessageLen;

ProtobufCodec::ProtobufCodec() {
}

ProtobufCodec::~ProtobufCodec() {
}

std::string ProtobufCodec::encode(const google::protobuf::Message& message) {
    std::string result;
    
    // 序列化消息
    std::string serialized;
    if (!message.SerializeToString(&serialized)) {
        LOG_ERROR("ProtobufCodec::encode - serialize failed");
        return result;
    }
    
    // 创建消息头：[4字节长度(网络字节序)][4字节类型(网络字节序)]
    int32_t len = static_cast<int32_t>(serialized.size());
    int32_t type = getMessageType(message);
    
    int32_t lenNet = htonl(len);
    int32_t typeNet = htonl(type);
    
    // 组装完整消息
    result.append(reinterpret_cast<char*>(&lenNet), sizeof(lenNet));
    result.append(reinterpret_cast<char*>(&typeNet), sizeof(typeNet));
    result.append(serialized);
    
    return result;
}

void ProtobufCodec::decode(const std::shared_ptr<TcpConnection>& conn, Buffer* buf) {
    while (buf->readableBytes() >= kHeaderLen) {
        // 检查消息完整性
        size_t messageLen = 0;
        if (!checkComplete(buf->peek(), buf->readableBytes(), &messageLen)) {
            break;
        }
        
        if (messageLen > kMaxMessageLen) {
            if (errorCallback_) {
                errorCallback_(conn, buf, 0, "message too large");
            }
            break;
        }
        
        // 提取消息头
        const char* data = buf->peek() + kHeaderLen;
        
        // 获取消息类型
        int32_t typeNet;
        std::memcpy(&typeNet, buf->peek() + sizeof(int32_t), sizeof(typeNet));
        int32_t type = ntohl(typeNet);
        
        // 创建消息对象
        ProtobufMessagePtr message = createMessage(type);
        if (!message) {
            if (errorCallback_) {
                errorCallback_(conn, buf, 0, "unknown message type");
            }
            buf->retrieve(messageLen + kHeaderLen);
            continue;
        }
        
        // 反序列化消息
        if (!message->ParseFromArray(data, messageLen)) {
            if (errorCallback_) {
                errorCallback_(conn, buf, 0, "parse message failed");
            }
            buf->retrieve(messageLen + kHeaderLen);
            continue;
        }
        
        // 调用消息回调
        if (messageCallback_) {
            messageCallback_(conn, message);
        }
        
        // 移除已处理的消息
        buf->retrieve(messageLen + kHeaderLen);
    }
}

bool ProtobufCodec::checkComplete(const char* data, size_t len, size_t* messageLen) {
    if (len < kHeaderLen) {
        return false;
    }
    
    // 读取消息长度
    int32_t lenNet;
    std::memcpy(&lenNet, data, sizeof(lenNet));
    int32_t len = ntohl(lenNet);
    
    *messageLen = len;
    
    // 检查完整消息是否都在缓冲区中
    if (len < 0 || static_cast<size_t>(len) > kMaxMessageLen) {
        return false;
    }
    
    return (len + kHeaderLen) <= len;
}

int ProtobufCodec::getMessageType(const google::protobuf::Message& message) {
    // 根据消息的描述符获取消息类型
    const std::string& typeName = message.GetTypeName();
    
    // 这里可以根据类型名称映射到消息类型
    // 简化实现：使用哈希值
    return static_cast<int>(std::hash<std::string>()(typeName) % 1000);
}

ProtobufCodec::ProtobufMessagePtr ProtobufCodec::createMessage(int messageType) {
    ProtobufMessageType type = static_cast<ProtobufMessageType>(messageType);
    
    // 这里需要根据消息类型创建对应的消息对象
    // 需要引用生成的protobuf消息类
    // 暂时返回nullptr，实际使用时需要包含对应的protobuf头文件
    
    switch (type) {
        case ProtobufMessageType::LOGIN_REQUEST:
        case ProtobufMessageType::LOGIN_RESPONSE:
        case ProtobufMessageType::REGISTER_REQUEST:
        case ProtobufMessageType::REGISTER_RESPONSE:
        case ProtobufMessageType::LOGOUT_REQUEST:
        case ProtobufMessageType::LOGOUT_RESPONSE:
        case ProtobufMessageType::CREATE_ROOM_REQUEST:
        case ProtobufMessageType::CREATE_ROOM_RESPONSE:
        case ProtobufMessageType::JOIN_ROOM_REQUEST:
        case ProtobufMessageType::JOIN_ROOM_RESPONSE:
        case ProtobufMessageType::LEAVE_ROOM_REQUEST:
        case ProtobufMessageType::LEAVE_ROOM_RESPONSE:
        case ProtobufMessageType::ROOM_LIST_REQUEST:
        case ProtobufMessageType::ROOM_LIST_RESPONSE:
        case ProtobufMessageType::PLAYER_JOINED_NOTIFY:
        case ProtobufMessageType::PLAYER_LEFT_NOTIFY:
        case ProtobufMessageType::ROOM_STATE_UPDATE_NOTIFY:
        case ProtobufMessageType::START_GAME_REQUEST:
        case ProtobufMessageType::START_GAME_RESPONSE:
        case ProtobufMessageType::GAME_STARTED_NOTIFY:
        case ProtobufMessageType::MOVE_REQUEST:
        case ProtobufMessageType::MOVE_RESPONSE:
        case ProtobufMessageType::MOVE_NOTIFY:
        case ProtobufMessageType::GAME_OVER_NOTIFY:
        case ProtobufMessageType::UNDO_REQUEST:
        case ProtobufMessageType::UNDO_RESPONSE:
        case ProtobufMessageType::RANDOM_MATCH_REQUEST:
        case ProtobufMessageType::MATCH_WAITING_RESPONSE:
        case ProtobufMessageType::MATCH_SUCCESS_NOTIFY:
        case ProtobufMessageType::CANCEL_MATCH_REQUEST:
        case ProtobufMessageType::CANCEL_MATCH_RESPONSE:
        case ProtobufMessageType::CHAT_MESSAGE_REQUEST:
        case ProtobufMessageType::CHAT_MESSAGE_NOTIFY:
        case ProtobufMessageType::SPECTATE_REQUEST:
        case ProtobufMessageType::SPECTATE_RESPONSE:
        case ProtobufMessageType::SPECTATOR_JOIN_NOTIFY:
        case ProtobufMessageType::SPECTATOR_LEFT_NOTIFY:
        case ProtobufMessageType::RANK_LIST_REQUEST:
        case ProtobufMessageType::RANK_LIST_RESPONSE:
        case ProtobufMessageType::HEARTBEAT_REQUEST:
        case ProtobufMessageType::HEARTBEAT_RESPONSE:
        case ProtobufMessageType::ERROR_NOTIFY:
            // TODO: 创建对应的消息对象
            return nullptr;
            
        default:
            return nullptr;
    }
}

void ProtobufCodec::setMessageCallback(const ProtobufMessageCallback& cb) {
    messageCallback_ = cb;
}

void ProtobufCodec::setErrorCallback(const ErrorCallback& cb) {
    errorCallback_ = cb;
}

void ProtobufCodec::defaultErrorCallback(const std::shared_ptr<TcpConnection>& conn, 
                                          Buffer* buf, 
                                          int errorCode, 
                                          const std::string& errorMessage) {
    LOG_ERROR("ProtobufCodec error: " + errorMessage + " (" + std::to_string(errorCode) + ")");
    // 可以选择关闭连接或发送错误通知
}

} // namespace gomoku