#include "business/MatchManager.h"
#include "business/RoomManager.h"
#include "utils/Logger.h"
#include <sstream>
#include <random>

namespace gomoku {

MatchManager::MatchManager() : redis_(nullptr), userManager_(nullptr), roomManager_(nullptr) {
}

MatchManager::~MatchManager() {
}

bool MatchManager::initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager) {
    redis_ = redis;
    userManager_ = userManager;
    roomManager_ = roomManager;
    
    if (!redis_ || !userManager_ || !roomManager_) {
        LOG_ERROR("MatchManager::initialize - redis, userManager or roomManager is null");
        return false;
    }
    
    // 加载现有匹配队列
    loadMatchQueueFromRedis();
    
    LOG_INFO("MatchManager initialized");
    return true;
}

void MatchManager::setBroadcastCallback(const BroadcastCallback& cb) {
    broadcastCallback_ = cb;
}

bool MatchManager::requestMatch(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查用户是否已在队列中
    std::ostringstream matchingKey;
    matchingKey << "matching_users:" << userId;
    if (redis_->exists(matchingKey.str())) {
        LOG_WARN("MatchManager::requestMatch - user already in queue: " + std::to_string(userId));
        return false;
    }
    
    // 添加到Redis匹配队列
    redis_->lpush("match_queue", std::to_string(userId));
    
    // 标记用户为匹配中
    redis_->set(matchingKey.str(), "1", 300); // 5分钟过期
    
    // 添加到本地队列
    matchQueue_.push(userId);
    
    LOG_INFO("MatchManager::requestMatch - user " + std::to_string(userId) + " added to match queue");
    
    // 尝试匹配
    return tryMatch();
}

bool MatchManager::cancelMatch(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从Redis匹配队列移除
    // 注意：Redis的list没有直接移除指定元素的方法，需要遍历
    // 这里简化处理，只移除标记
    
    std::ostringstream matchingKey;
    matchingKey << "matching_users:" << userId;
    redis_->del(matchingKey.str());
    
    LOG_INFO("MatchManager::cancelMatch - user " + std::to_string(userId) + " cancelled match");
    return true;
}

int MatchManager::getQueuePosition(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从Redis获取队列位置
    auto queue = redis_->lrange("match_queue", 0, -1);
    
    for (size_t i = 0; i < queue.size(); ++i) {
        if (queue[i] == std::to_string(userId)) {
            return static_cast<int>(i + 1);
        }
    }
    
    return 0;
}

bool MatchManager::tryMatch() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从Redis获取匹配队列
    auto queue = redis_->lrange("match_queue", 0, 1); // 获取前两个
    
    if (queue.size() < 2) {
        return false;
    }
    
    int user1 = std::stoi(queue[0]);
    int user2 = std::stoi(queue[1]);
    
    // 创建匹配房间
    std::string roomId;
    if (!createMatchRoom(user1, user2, roomId)) {
        return false;
    }
    
    // 从队列移除这两个用户
    redis_->lpop("match_queue"); // user1
    redis_->lpop("match_queue"); // user2
    
    // 移除匹配标记
    std::ostringstream key1, key2;
    key1 << "matching_users:" << user1;
    key2 << "matching_users:" << user2;
    redis_->del(key1.str());
    redis_->del(key2.str());
    
    // 更新本地队列
    if (!matchQueue_.empty()) {
        matchQueue_.pop();
    }
    if (!matchQueue_.empty()) {
        matchQueue_.pop();
    }
    
    // 通知匹配成功
    notifyMatchSuccess(user1, user2, roomId);
    
    LOG_INFO("MatchManager::tryMatch - matched user " + std::to_string(user1) + " with user " + std::to_string(user2));
    return true;
}

bool MatchManager::createMatchRoom(int user1, int user2, std::string& roomId) {
    if (!roomManager_) {
        return false;
    }
    
    // 生成随机房间名
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    std::string roomName = "匹配房间-" + std::to_string(dis(gen));
    
    // 创建房间
    RoomInfo roomInfo;
    if (!roomManager_->createRoom(user1, roomName, "", roomInfo)) {
        LOG_ERROR("MatchManager::createMatchRoom - failed to create room");
        return false;
    }
    
    roomId = roomInfo.roomId;
    
    // 第二个玩家加入
    RoomInfo joinInfo;
    if (!roomManager_->joinRoom(user2, roomId, "", joinInfo)) {
        LOG_ERROR("MatchManager::createMatchRoom - failed to join room");
        return false;
    }
    
    return true;
}

void MatchManager::notifyMatchSuccess(int user1, int user2, const std::string& roomId) {
    // 构造匹配成功通知消息
    std::ostringstream msg1, msg2;
    
    // 获取对手信息
    UserInfo opponent1, opponent2;
    if (userManager_->getUserInfo(user2, opponent1)) {
        msg1 << "MATCH_SUCCESS:" << roomId << ":" << opponent1.nickname;
    } else {
        msg1 << "MATCH_SUCCESS:" << roomId << ":Unknown";
    }
    
    if (userManager_->getUserInfo(user1, opponent2)) {
        msg2 << "MATCH_SUCCESS:" << roomId << ":" << opponent2.nickname;
    } else {
        msg2 << "MATCH_SUCCESS:" << roomId << ":Unknown";
    }
    
    // 发送通知
    if (broadcastCallback_) {
        broadcastCallback_(user1, msg1.str());
        broadcastCallback_(user2, msg2.str());
    }
}

void MatchManager::loadMatchQueueFromRedis() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto queue = redis_->lrange("match_queue", 0, -1);
    
    for (const auto& userIdStr : queue) {
        int userId = std::stoi(userIdStr);
        matchQueue_.push(userId);
    }
    
    LOG_INFO("MatchManager::loadMatchQueueFromRedis - loaded " + std::to_string(matchQueue_.size()) + " users");
}

} // namespace gomoku