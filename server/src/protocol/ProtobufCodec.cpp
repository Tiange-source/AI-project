#include "protocol/ProtobufCodec.h"
#include "network/TcpConnection.h"
#include "network/Buffer.h"
#include "utils/Logger.h"
#include <arpa/inet.h>
#include <cstring>

// 包含protobuf生成的头文件
#include "gomoku.pb.h"

namespace gomoku {

// 静态成员初始化
std::unordered_map<int, std::function<ProtobufCodec::ProtobufMessagePtr()>> ProtobufCodec::messageFactories_;
std::unordered_map<int, std::string> ProtobufCodec::messageTypeNames_;

const int ProtobufCodec::kHeaderLen;
const int ProtobufCodec::kMaxMessageLen;

ProtobufCodec::ProtobufCodec() {
    // 静态初始化消息工厂（只执行一次）
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        
        // 注册消息工厂
        registerMessageFactory<gomoku::LoginRequest>(static_cast<int>(gomoku::MessageType::LOGIN_REQUEST));
        registerMessageFactory<gomoku::LoginResponse>(static_cast<int>(gomoku::MessageType::LOGIN_RESPONSE));
        registerMessageFactory<gomoku::RegisterRequest>(static_cast<int>(gomoku::MessageType::REGISTER_REQUEST));
        registerMessageFactory<gomoku::RegisterResponse>(static_cast<int>(gomoku::MessageType::REGISTER_RESPONSE));
        registerMessageFactory<gomoku::CreateRoomRequest>(static_cast<int>(gomoku::MessageType::CREATE_ROOM_REQUEST));
        registerMessageFactory<gomoku::CreateRoomResponse>(static_cast<int>(gomoku::MessageType::CREATE_ROOM_RESPONSE));
        registerMessageFactory<gomoku::JoinRoomRequest>(static_cast<int>(gomoku::MessageType::JOIN_ROOM_REQUEST));
        registerMessageFactory<gomoku::JoinRoomResponse>(static_cast<int>(gomoku::MessageType::JOIN_ROOM_RESPONSE));
        registerMessageFactory<gomoku::LeaveRoomRequest>(static_cast<int>(gomoku::MessageType::LEAVE_ROOM_REQUEST));
        registerMessageFactory<gomoku::RoomListRequest>(static_cast<int>(gomoku::MessageType::ROOM_LIST_REQUEST));
        registerMessageFactory<gomoku::RoomListResponse>(static_cast<int>(gomoku::MessageType::ROOM_LIST_RESPONSE));
        registerMessageFactory<gomoku::StartGameRequest>(static_cast<int>(gomoku::MessageType::START_GAME_REQUEST));
        registerMessageFactory<gomoku::MoveRequest>(static_cast<int>(gomoku::MessageType::MOVE_REQUEST));
        registerMessageFactory<gomoku::MoveResponse>(static_cast<int>(gomoku::MessageType::MOVE_RESPONSE));
        registerMessageFactory<gomoku::MoveNotify>(static_cast<int>(gomoku::MessageType::MOVE_NOTIFY));
        registerMessageFactory<gomoku::GameOverNotify>(static_cast<int>(gomoku::MessageType::GAME_OVER_NOTIFY));
        registerMessageFactory<gomoku::RandomMatchRequest>(static_cast<int>(gomoku::MessageType::RANDOM_MATCH_REQUEST));
        registerMessageFactory<gomoku::ChatMessageRequest>(static_cast<int>(gomoku::MessageType::CHAT_MESSAGE_REQUEST));
        registerMessageFactory<gomoku::ChatMessageNotify>(static_cast<int>(gomoku::MessageType::CHAT_MESSAGE_NOTIFY));
        registerMessageFactory<gomoku::SpectateRequest>(static_cast<int>(gomoku::MessageType::SPECTATE_REQUEST));
        registerMessageFactory<gomoku::SpectateResponse>(static_cast<int>(gomoku::MessageType::SPECTATE_RESPONSE));
        registerMessageFactory<gomoku::RankListRequest>(static_cast<int>(gomoku::MessageType::RANK_LIST_REQUEST));
        registerMessageFactory<gomoku::RankListResponse>(static_cast<int>(gomoku::MessageType::RANK_LIST_RESPONSE));
        
        LOG_INFO("ProtobufCodec message factories initialized");
    }
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
                errorCallback_(conn, buf, 0, "unknown message type: " + std::to_string(type));
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
    
    // 根据类型名称映射到消息类型
    // 从protobuf文件中定义的枚举值
    if (typeName == "gomoku.LoginRequest") return static_cast<int>(gomoku::MessageType::LOGIN_REQUEST);
    if (typeName == "gomoku.LoginResponse") return static_cast<int>(gomoku::MessageType::LOGIN_RESPONSE);
    if (typeName == "gomoku.RegisterRequest") return static_cast<int>(gomoku::MessageType::REGISTER_REQUEST);
    if (typeName == "gomoku.RegisterResponse") return static_cast<int>(gomoku::MessageType::REGISTER_RESPONSE);
    if (typeName == "gomoku.CreateRoomRequest") return static_cast<int>(gomoku::MessageType::CREATE_ROOM_REQUEST);
    if (typeName == "gomoku.CreateRoomResponse") return static_cast<int>(gomoku::MessageType::CREATE_ROOM_RESPONSE);
    if (typeName == "gomoku.JoinRoomRequest") return static_cast<int>(gomoku::MessageType::JOIN_ROOM_REQUEST);
    if (typeName == "gomoku.JoinRoomResponse") return static_cast<int>(gomoku::MessageType::JOIN_ROOM_RESPONSE);
    if (typeName == "gomoku.LeaveRoomRequest") return static_cast<int>(gomoku::MessageType::LEAVE_ROOM_REQUEST);
    if (typeName == "gomoku.RoomListRequest") return static_cast<int>(gomoku::MessageType::ROOM_LIST_REQUEST);
    if (typeName == "gomoku.RoomListResponse") return static_cast<int>(gomoku::MessageType::ROOM_LIST_RESPONSE);
    if (typeName == "gomoku.StartGameRequest") return static_cast<int>(gomoku::MessageType::START_GAME_REQUEST);
    if (typeName == "gomoku.MoveRequest") return static_cast<int>(gomoku::MessageType::MOVE_REQUEST);
    if (typeName == "gomoku.MoveResponse") return static_cast<int>(gomoku::MessageType::MOVE_RESPONSE);
    if (typeName == "gomoku.MoveNotify") return static_cast<int>(gomoku::MessageType::MOVE_NOTIFY);
    if (typeName == "gomoku.GameOverNotify") return static_cast<int>(gomoku::MessageType::GAME_OVER_NOTIFY);
    if (typeName == "gomoku.RandomMatchRequest") return static_cast<int>(gomoku::MessageType::RANDOM_MATCH_REQUEST);
    if (typeName == "gomoku.ChatMessageRequest") return static_cast<int>(gomoku::MessageType::CHAT_MESSAGE_REQUEST);
    if (typeName == "gomoku.ChatMessageNotify") return static_cast<int>(gomoku::MessageType::CHAT_MESSAGE_NOTIFY);
    if (typeName == "gomoku.SpectateRequest") return static_cast<int>(gomoku::MessageType::SPECTATE_REQUEST);
    if (typeName == "gomoku.SpectateResponse") return static_cast<int>(gomoku::MessageType::SPECTATE_RESPONSE);
    if (typeName == "gomoku.RankListRequest") return static_cast<int>(gomoku::MessageType::RANK_LIST_REQUEST);
    if (typeName == "gomoku.RankListResponse") return static_cast<int>(gomoku::MessageType::RANK_LIST_RESPONSE);
    
    // 未知类型
    LOG_WARN("Unknown message type: " + typeName);
    return 0;
}

ProtobufCodec::ProtobufMessagePtr ProtobufCodec::createMessage(int messageType) {
    auto it = messageFactories_.find(messageType);
    if (it != messageFactories_.end()) {
        return it->second();
    }
    
    LOG_WARN("No factory registered for message type: " + std::to_string(messageType));
    return nullptr;
}

void ProtobufCodec::registerMessageType(int messageType, const std::string& typeName) {
    messageTypeNames_[messageType] = typeName;
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