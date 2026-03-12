#include "business/ChatManager.h"
#include "utils/Logger.h"
#include <sstream>
#include <chrono>
#include <algorithm>

namespace gomoku {

ChatManager::ChatManager() : redis_(nullptr), userManager_(nullptr), roomManager_(nullptr) {
}

ChatManager::~ChatManager() {
}

bool ChatManager::initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager) {
    redis_ = redis;
    userManager_ = userManager;
    roomManager_ = roomManager;
    
    if (!redis_ || !userManager_ || !roomManager_) {
        LOG_ERROR("ChatManager::initialize - redis, userManager or roomManager is null");
        return false;
    }
    
    LOG_INFO("ChatManager initialized");
    return true;
}

void ChatManager::setBroadcastCallback(const BroadcastCallback& cb) {
    broadcastCallback_ = cb;
}

bool ChatManager::sendLobbyChat(int senderId, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取发送者信息
    InternalUserInfo senderInfo;
    if (!userManager_->getUserInfo(senderId, senderInfo)) {
        LOG_WARN("ChatManager::sendLobbyChat - sender not found: " + std::to_string(senderId));
        return false;
    }
    
    // 敏感词过滤
    std::string filteredContent = filterSensitiveWords(content);
    
    // 保存消息
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    saveMessage(InternalChatType::LOBBY, "lobby", senderId, senderInfo.nickname, filteredContent, timestamp);
    
    // 获取所有在线用户
    auto onlineUsers = userManager_->getOnlineUsers();
    std::vector<int> userIds;
    for (const auto& user : onlineUsers) {
        userIds.push_back(user.userId);
    }
    
    // 构造消息
    std::string message = formatMessage(senderId, senderInfo.nickname, filteredContent, timestamp);
    message = "LOBBY_CHAT:" + message;
    
    // 广播消息
    if (broadcastCallback_) {
        broadcastCallback_(userIds, message);
    }
    
    LOG_INFO("ChatManager::sendLobbyChat - message sent by user " + std::to_string(senderId));
    return true;
}

bool ChatManager::sendPrivateChat(int senderId, int receiverId, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取发送者信息
    InternalUserInfo senderInfo;
    if (!userManager_->getUserInfo(senderId, senderInfo)) {
        LOG_WARN("ChatManager::sendPrivateChat - sender not found: " + std::to_string(senderId));
        return false;
    }
    
    // 检查接收者是否在线
    if (!userManager_->isUserOnline(receiverId)) {
        LOG_WARN("ChatManager::sendPrivateChat - receiver not online: " + std::to_string(receiverId));
        return false;
    }
    
    // 敏感词过滤
    std::string filteredContent = filterSensitiveWords(content);
    
    // 保存消息（双向存储）
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    std::string key1 = "private:" + std::to_string(senderId) + ":" + std::to_string(receiverId);
    std::string key2 = "private:" + std::to_string(receiverId) + ":" + std::to_string(senderId);
    
    saveMessage(InternalChatType::PRIVATE, key1, senderId, senderInfo.nickname, filteredContent, timestamp);
    saveMessage(InternalChatType::PRIVATE, key2, senderId, senderInfo.nickname, filteredContent, timestamp);
    
    // 构造消息
    std::string message = formatMessage(senderId, senderInfo.nickname, filteredContent, timestamp);
    message = "PRIVATE_CHAT:" + std::to_string(senderId) + ":" + message;
    
    // 发送给接收者
    if (broadcastCallback_) {
        broadcastCallback_({receiverId}, message);
    }
    
    LOG_INFO("ChatManager::sendPrivateChat - message sent from user " + std::to_string(senderId) + " to user " + std::to_string(receiverId));
    return true;
}

bool ChatManager::sendRoomChat(int senderId, const std::string& roomId, const std::string& content) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 获取发送者信息
    InternalUserInfo senderInfo;
    if (!userManager_->getUserInfo(senderId, senderInfo)) {
        LOG_WARN("ChatManager::sendRoomChat - sender not found: " + std::to_string(senderId));
        return false;
    }
    
    // 检查房间是否存在
    InternalRoomInfo roomInfo;
    if (!roomManager_->getRoomInfo(roomId, roomInfo)) {
        LOG_WARN("ChatManager::sendRoomChat - room not found: " + roomId);
        return false;
    }
    
    // 敏感词过滤
    std::string filteredContent = filterSensitiveWords(content);
    
    // 保存消息
    long long timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    saveMessage(InternalChatType::ROOM, roomId, senderId, senderInfo.nickname, filteredContent, timestamp);
    
    // 获取房间内所有用户
    auto spectators = roomManager_->getSpectators(roomId);
    std::vector<int> userIds;
    if (roomInfo.player1 != 0) {
        userIds.push_back(roomInfo.player1);
    }
    if (roomInfo.player2 != 0) {
        userIds.push_back(roomInfo.player2);
    }
    userIds.insert(userIds.end(), spectators.begin(), spectators.end());
    
    // 构造消息
    std::string message = formatMessage(senderId, senderInfo.nickname, filteredContent, timestamp);
    message = "ROOM_CHAT:" + roomId + ":" + message;
    
    // 广播消息到房间
    if (broadcastCallback_) {
        broadcastCallback_(userIds, message);
    }
    
    LOG_INFO("ChatManager::sendRoomChat - message sent in room " + roomId + " by user " + std::to_string(senderId));
    return true;
}

std::vector<std::string> ChatManager::getHistory(InternalChatType type, const std::string& id, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream key;
    switch (type) {
        case InternalChatType::LOBBY:
            key << "chat:lobby";
            break;
        case InternalChatType::ROOM:
            key << "chat:room:" << id;
            break;
        case InternalChatType::PRIVATE:
            key << "chat:private:" << id;
            break;
    }
    
    // 获取最近N条消息
    auto messages = redis_->zrange(key.str(), -limit, -1);
    
    return messages;
}

std::string ChatManager::filterSensitiveWords(const std::string& content) {
    std::string filtered = content;
    
    // 简化的敏感词过滤
    std::vector<std::string> sensitiveWords = {"脏话", "暴力", "广告"};
    
    for (const auto& word : sensitiveWords) {
        size_t pos = 0;
        while ((pos = filtered.find(word, pos)) != std::string::npos) {
            filtered.replace(pos, word.length(), "***");
            pos += 3;
        }
    }
    
    return filtered;
}

bool ChatManager::saveMessage(InternalChatType type, const std::string& id, int senderId, const std::string& senderName,
                               const std::string& content, long long timestamp) {
    std::ostringstream key;
    switch (type) {
        case InternalChatType::LOBBY:
            key << "chat:lobby";
            break;
        case InternalChatType::ROOM:
            key << "chat:room:" << id;
            break;
        case InternalChatType::PRIVATE:
            key << "chat:private:" << id;
            break;
    }
    
    // 构造JSON格式的消息
    std::ostringstream messageJson;
    messageJson << "{\"sender_id\":" << senderId
                << ",\"sender_name\":\"" << senderName
                << "\",\"content\":\"" << content
                << "\",\"timestamp\":" << timestamp << "}";
    
    // 添加到有序集合（按时间戳排序）
    redis_->zadd(key.str(), static_cast<double>(timestamp), messageJson.str());
    
    // 保持最多1000条消息
    int count = redis_->zcard(key.str());
    if (count > 1000) {
        redis_->zremrangebyrank(key.str(), 0, count - 1001);
    }
    
    return true;
}

std::string ChatManager::formatMessage(int senderId, const std::string& senderName, const std::string& content,
                                        long long timestamp) {
    std::ostringstream message;
    message << senderId << ":" << senderName << ":" << content << ":" << timestamp;
    return message.str();
}

} // namespace gomoku