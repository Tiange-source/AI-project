#ifndef CHAT_MANAGER_H
#define CHAT_MANAGER_H

#include "storage/RedisClient.h"
#include "business/UserManager.h"
#include "business/RoomManager.h"
#include <string>
#include <mutex>

namespace gomoku {

enum class InternalChatType {
    LOBBY,
    PRIVATE,
    ROOM
};

class ChatManager {
public:
    using BroadcastCallback = std::function<void(const std::vector<int>& userIds, const std::string& message)>;
    
    ChatManager();
    ~ChatManager();
    
    // 初始化
    bool initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager);
    
    // 设置广播回调
    void setBroadcastCallback(const BroadcastCallback& cb);
    
    // 发送大厅聊天
    bool sendLobbyChat(int senderId, const std::string& content);
    
    // 发送私聊
    bool sendPrivateChat(int senderId, int receiverId, const std::string& content);
    
    // 发送房间聊天
    bool sendRoomChat(int senderId, const std::string& roomId, const std::string& content);
    
    // 获取聊天历史
    std::vector<std::string> getHistory(InternalChatType type, const std::string& id, int limit = 50);
    
    // 敏感词过滤
    std::string filterSensitiveWords(const std::string& content);

private:
    // 保存消息到Redis
    bool saveMessage(InternalChatType type, const std::string& id, int senderId, const std::string& senderName, 
                     const std::string& content, long long timestamp);
    
    // 格式化消息
    std::string formatMessage(int senderId, const std::string& senderName, const std::string& content,
                              long long timestamp);

    RedisClient* redis_;
    UserManager* userManager_;
    RoomManager* roomManager_;
    std::mutex mutex_;
    BroadcastCallback broadcastCallback_;
};

} // namespace gomoku

#endif // CHAT_MANAGER_H