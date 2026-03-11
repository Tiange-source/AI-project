#ifndef PROTOBUF_CODEC_H
#define PROTOBUF_CODEC_H

#include <google/protobuf/message.h>
#include <functional>
#include <string>
#include <memory>

namespace gomoku {

class TcpConnection;
class Buffer;

// Protobuf消息类型
enum class ProtobufMessageType {
    UNKNOWN = 0,
    LOGIN_REQUEST = 1,
    LOGIN_RESPONSE = 2,
    REGISTER_REQUEST = 3,
    REGISTER_RESPONSE = 4,
    LOGOUT_REQUEST = 5,
    LOGOUT_RESPONSE = 6,
    CREATE_ROOM_REQUEST = 100,
    CREATE_ROOM_RESPONSE = 101,
    JOIN_ROOM_REQUEST = 102,
    JOIN_ROOM_RESPONSE = 103,
    LEAVE_ROOM_REQUEST = 104,
    LEAVE_ROOM_RESPONSE = 105,
    ROOM_LIST_REQUEST = 106,
    ROOM_LIST_RESPONSE = 107,
    PLAYER_JOINED_NOTIFY = 108,
    PLAYER_LEFT_NOTIFY = 109,
    ROOM_STATE_UPDATE_NOTIFY = 110,
    START_GAME_REQUEST = 200,
    START_GAME_RESPONSE = 201,
    GAME_STARTED_NOTIFY = 202,
    MOVE_REQUEST = 203,
    MOVE_RESPONSE = 204,
    MOVE_NOTIFY = 205,
    GAME_OVER_NOTIFY = 206,
    UNDO_REQUEST = 207,
    UNDO_RESPONSE = 208,
    RANDOM_MATCH_REQUEST = 300,
    MATCH_WAITING_RESPONSE = 301,
    MATCH_SUCCESS_NOTIFY = 302,
    CANCEL_MATCH_REQUEST = 303,
    CANCEL_MATCH_RESPONSE = 304,
    CHAT_MESSAGE_REQUEST = 400,
    CHAT_MESSAGE_NOTIFY = 401,
    SPECTATE_REQUEST = 500,
    SPECTATE_RESPONSE = 501,
    SPECTATOR_JOIN_NOTIFY = 502,
    SPECTATOR_LEFT_NOTIFY = 503,
    RANK_LIST_REQUEST = 600,
    RANK_LIST_RESPONSE = 601,
    HEARTBEAT_REQUEST = 700,
    HEARTBEAT_RESPONSE = 701,
    ERROR_NOTIFY = 702
};

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
    
    static const int kHeaderLen = sizeof(int32_t) + sizeof(int32_t);  // [长度][类型]
    static const int kMaxMessageLen = 64 * 1024 * 1024;  // 64MB
};

} // namespace gomoku

#endif // PROTOBUF_CODEC_H