#include "business/SpectatorManager.h"
#include "utils/Logger.h"

namespace gomoku {

SpectatorManager::SpectatorManager() : redis_(nullptr), userManager_(nullptr), roomManager_(nullptr) {
}

SpectatorManager::~SpectatorManager() {
}

bool SpectatorManager::initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager) {
    redis_ = redis;
    userManager_ = userManager;
    roomManager_ = roomManager;
    
    if (!redis_ || !userManager_ || !roomManager_) {
        LOG_ERROR("SpectatorManager::initialize - redis, userManager or roomManager is null");
        return false;
    }
    
    LOG_INFO("SpectatorManager initialized");
    return true;
}

bool SpectatorManager::addSpectator(const std::string& roomId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查房间是否存在
    RoomInfo roomInfo;
    if (!roomManager_->getRoomInfo(roomId, roomInfo)) {
        LOG_WARN("SpectatorManager::addSpectator - room not found: " + roomId);
        return false;
    }
    
    // 检查房间是否正在游戏中
    if (roomInfo.state != RoomState::GAMING) {
        LOG_WARN("SpectatorManager::addSpectator - room not in gaming state: " + roomId);
        return false;
    }
    
    // 检查用户是否已经在房间中
    if (roomInfo.player1 == userId || roomInfo.player2 == userId) {
        LOG_WARN("SpectatorManager::addSpectator - user is already a player in room: " + roomId);
        return false;
    }
    
    // 添加到观战者列表
    std::ostringstream key;
    key << "room:" << roomId << ":spectators";
    redis_->sadd(key.str(), std::to_string(userId));
    
    LOG_INFO("SpectatorManager::addSpectator - user " + std::to_string(userId) + " added to spectators in room " + roomId);
    return true;
}

bool SpectatorManager::removeSpectator(const std::string& roomId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从观战者列表移除
    std::ostringstream key;
    key << "room:" << roomId << ":spectators";
    redis_->srem(key.str(), std::to_string(userId));
    
    LOG_INFO("SpectatorManager::removeSpectator - user " + std::to_string(userId) + " removed from spectators in room " + roomId);
    return true;
}

std::vector<int> SpectatorManager::getSpectators(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream key;
    key << "room:" << roomId << ":spectators";
    
    auto spectatorStrs = redis_->smembers(key.str());
    
    std::vector<int> spectators;
    for (const auto& str : spectatorStrs) {
        try {
            spectators.push_back(std::stoi(str));
        } catch (...) {
            // 忽略无效数据
        }
    }
    
    return spectators;
}

std::vector<std::string> SpectatorManager::getMoveHistory(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ostringstream key;
    key << "room:" << roomId << ":moves";
    
    auto moves = redis_->lrange(key.str(), 0, -1);
    
    return moves;
}

} // namespace gomoku