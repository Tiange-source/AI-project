#include "business/RoomManager.h"
#include "utils/Logger.h"
#include <sstream>
#include <random>
#include <ctime>

namespace gomoku {

RoomManager::RoomManager() : redis_(nullptr), userManager_(nullptr) {
}

RoomManager::~RoomManager() {
}

bool RoomManager::initialize(RedisClient* redis, UserManager* userManager) {
    redis_ = redis;
    userManager_ = userManager;
    
    if (!redis_ || !userManager_) {
        LOG_ERROR("RoomManager::initialize - redis or userManager is null");
        return false;
    }
    
    LOG_INFO("RoomManager initialized");
    return true;
}

void RoomManager::setBroadcastCallback(const BroadcastCallback& cb) {
    broadcastCallback_ = cb;
}

bool RoomManager::createRoom(int ownerId, const std::string& roomName, const std::string& password, 
                              InternalRoomInfo& roomInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 生成房间号
    std::string roomId = generateRoomId();
    
    // 创建房间信息
    InternalRoomInfo info;
    info.roomId = roomId;
    info.roomName = roomName;
    info.ownerId = ownerId;
    info.password = password;
    info.hasPassword = !password.empty();
    info.state = InternalRoomState::WAITING;
    info.player1 = ownerId;
    info.player2 = 0;
    info.spectators.clear();
    info.currentTurn = ownerId;
    info.startTime = std::time(nullptr);
    
    // 保存到Redis
    if (!saveRoomToRedis(info)) {
        LOG_ERROR("RoomManager::createRoom - failed to save room to redis");
        return false;
    }
    
    // 添加到活跃房间集合
    redis_->sadd("rooms:active", roomId);
    
    // 保存房主映射
    std::ostringstream ownerKey;
    ownerKey << "room:owner:" << ownerId;
    redis_->set(ownerKey.str(), roomId);
    
    // 添加到房间成员集合
    std::ostringstream membersKey;
    membersKey << "room:" << roomId << ":members";
    redis_->sadd(membersKey.str(), std::to_string(ownerId));
    
    // 本地缓存
    rooms_[roomId] = info;
    
    roomInfo = info;
    
    LOG_INFO("RoomManager::createRoom - room created: " + roomId + " by user " + std::to_string(ownerId));
    return true;
}

bool RoomManager::joinRoom(int userId, const std::string& roomId, const std::string& password, 
                           InternalRoomInfo& roomInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 加载房间信息
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        LOG_WARN("RoomManager::joinRoom - room not found: " + roomId);
        return false;
    }
    
    // 检查房间状态
    if (info.state != InternalRoomState::WAITING) {
        LOG_WARN("RoomManager::joinRoom - room is not waiting: " + roomId);
        return false;
    }
    
    // 检查房间是否已满
    if (isRoomFull(roomId)) {
        LOG_WARN("RoomManager::joinRoom - room is full: " + roomId);
        return false;
    }
    
    // 验证密码
    if (info.hasPassword && !validatePassword(roomId, password)) {
        LOG_WARN("RoomManager::joinRoom - invalid password for room: " + roomId);
        return false;
    }
    
    // 添加第二个玩家
    if (info.player2 == 0) {
        info.player2 = userId;
    } else {
        // 作为观战者加入
        info.spectators.push_back(userId);
    }
    
    // 保存到Redis
    saveRoomToRedis(info);
    
    // 添加到房间成员集合
    std::ostringstream membersKey;
    membersKey << "room:" << roomId << ":members";
    redis_->sadd(membersKey.str(), std::to_string(userId));
    
    // 如果是第二个玩家，更新Redis
    if (info.player2 == userId) {
        std::ostringstream player2Key;
        player2Key << "room:" << roomId << ":player2";
        redis_->set(player2Key.str(), std::to_string(userId));
    }
    
    // 更新本地缓存
    rooms_[roomId] = info;
    
    roomInfo = info;
    
    LOG_INFO("RoomManager::joinRoom - user " + std::to_string(userId) + " joined room " + roomId);
    return true;
}

bool RoomManager::leaveRoom(int userId, const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    // 如果是房主
    if (info.ownerId == userId) {
        // 关闭房间
        return closeRoom(roomId);
    }
    
    // 从房间中移除
    bool removed = false;
    if (info.player2 == userId) {
        info.player2 = 0;
        removed = true;
    } else {
        // 检查观战者列表
        auto it = std::find(info.spectators.begin(), info.spectators.end(), userId);
        if (it != info.spectators.end()) {
            info.spectators.erase(it);
            removed = true;
        }
    }
    
    if (!removed) {
        return false;
    }
    
    // 保存到Redis
    saveRoomToRedis(info);
    
    // 从成员集合移除
    std::ostringstream membersKey;
    membersKey << "room:" << roomId << ":members";
    redis_->srem(membersKey.str(), std::to_string(userId));
    
    // 更新本地缓存
    rooms_[roomId] = info;
    
    LOG_INFO("RoomManager::leaveRoom - user " + std::to_string(userId) + " left room " + roomId);
    return true;
}

bool RoomManager::closeRoom(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从Redis移除房间
    removeRoomFromRedis(roomId);
    
    // 从活跃房间集合移除
    redis_->srem("rooms:active", roomId);
    
    // 移除本地缓存
    rooms_.erase(roomId);
    
    LOG_INFO("RoomManager::closeRoom - room closed: " + roomId);
    return true;
}

bool RoomManager::getRoomInfo(const std::string& roomId, InternalRoomInfo& roomInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 先从本地缓存查找
    auto it = rooms_.find(roomId);
    if (it != rooms_.end()) {
        roomInfo = it->second;
        return true;
    }
    
    // 从Redis加载
    return loadRoomFromRedis(roomId, roomInfo);
}

std::vector<InternalRoomInfo> RoomManager::getActiveRooms() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<InternalRoomInfo> rooms;
    
    // 从Redis获取所有活跃房间
    auto roomIds = redis_->smembers("rooms:active");
    
    for (const auto& roomId : roomIds) {
        InternalRoomInfo info;
        if (loadRoomFromRedis(roomId, info)) {
            rooms.push_back(info);
        }
    }
    
    return rooms;
}

bool RoomManager::isRoomFull(const std::string& roomId) {
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    return info.player1 != 0 && info.player2 != 0;
}

bool RoomManager::isRoomGaming(const std::string& roomId) {
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    return info.state == InternalRoomState::GAMING;
}

bool RoomManager::startGame(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    if (info.state != InternalRoomState::WAITING) {
        return false;
    }
    
    if (info.player1 == 0 || info.player2 == 0) {
        return false;
    }
    
    info.state = InternalRoomState::GAMING;
    info.startTime = std::time(nullptr);
    
    saveRoomToRedis(info);
    rooms_[roomId] = info;
    
    LOG_INFO("RoomManager::startGame - game started in room: " + roomId);
    return true;
}

bool RoomManager::updateRoomState(const std::string& roomId, InternalRoomState state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    info.state = state;
    
    saveRoomToRedis(info);
    rooms_[roomId] = info;
    
    return true;
}

bool RoomManager::addSpectator(const std::string& roomId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    // 检查是否已经在观战者列表中
    if (std::find(info.spectators.begin(), info.spectators.end(), userId) != info.spectators.end()) {
        return false;
    }
    
    info.spectators.push_back(userId);
    
    saveRoomToRedis(info);
    rooms_[roomId] = info;
    
    // 添加到观战者集合
    std::ostringstream spectatorsKey;
    spectatorsKey << "room:" << roomId << ":spectators";
    redis_->sadd(spectatorsKey.str(), std::to_string(userId));
    
    return true;
}

bool RoomManager::removeSpectator(const std::string& roomId, int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    auto it = std::find(info.spectators.begin(), info.spectators.end(), userId);
    if (it == info.spectators.end()) {
        return false;
    }
    
    info.spectators.erase(it);
    
    saveRoomToRedis(info);
    rooms_[roomId] = info;
    
    // 从观战者集合移除
    std::ostringstream spectatorsKey;
    spectatorsKey << "room:" << roomId << ":spectators";
    redis_->srem(spectatorsKey.str(), std::to_string(userId));
    
    return true;
}

std::vector<int> RoomManager::getSpectators(const std::string& roomId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return {};
    }
    
    return info.spectators;
}

bool RoomManager::broadcastToRoom(const std::string& roomId, const std::string& message) {
    auto userIds = getRoomUsers(roomId);
    
    if (userIds.empty()) {
        return false;
    }
    
    if (broadcastCallback_) {
        broadcastCallback_(userIds, message);
        return true;
    }
    
    return false;
}

std::string RoomManager::generateRoomId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100000, 999999);
    
    std::string roomId;
    do {
        roomId = std::to_string(dis(gen));
        // 检查房间号是否已存在
        InternalRoomInfo info;
        if (!getRoomInfo(roomId, info)) {
            break;
        }
    } while (true);
    
    return roomId;
}

bool RoomManager::loadRoomFromRedis(const std::string& roomId, InternalRoomInfo& roomInfo) {
    std::ostringstream key;
    key << "room:" << roomId;
    
    auto fields = redis_->hgetAll(key.str());
    if (fields.empty()) {
        return false;
    }
    
    for (const auto& pair : fields) {
        if (pair.first == "room_id") {
            roomInfo.roomId = pair.second;
        } else if (pair.first == "room_name") {
            roomInfo.roomName = pair.second;
        } else if (pair.first == "owner_id") {
            roomInfo.ownerId = std::stoi(pair.second);
        } else if (pair.first == "password") {
            roomInfo.password = pair.second;
        } else if (pair.first == "state") {
            roomInfo.state = static_cast<InternalRoomState>(std::stoi(pair.second));
        } else if (pair.first == "player1") {
            roomInfo.player1 = std::stoi(pair.second);
        } else if (pair.first == "player2") {
            roomInfo.player2 = std::stoi(pair.second);
        } else if (pair.first == "current_turn") {
            roomInfo.currentTurn = std::stoi(pair.second);
        } else if (pair.first == "start_time") {
            roomInfo.startTime = std::stoll(pair.second);
        }
    }
    
    roomInfo.hasPassword = !roomInfo.password.empty();
    
    // 加载观战者列表
    std::ostringstream spectatorsKey;
    spectatorsKey << "room:" << roomId << ":spectators";
    auto spectatorIds = redis_->smembers(spectatorsKey.str());
    for (const auto& id : spectatorIds) {
        roomInfo.spectators.push_back(std::stoi(id));
    }
    
    return true;
}

bool RoomManager::saveRoomToRedis(const InternalRoomInfo& roomInfo) {
    std::ostringstream key;
    key << "room:" << roomInfo.roomId;
    
    redis_->hset(key.str(), "room_id", roomInfo.roomId);
    redis_->hset(key.str(), "room_name", roomInfo.roomName);
    redis_->hset(key.str(), "owner_id", std::to_string(roomInfo.ownerId));
    redis_->hset(key.str(), "password", roomInfo.password);
    redis_->hset(key.str(), "state", std::to_string(static_cast<int>(roomInfo.state)));
    redis_->hset(key.str(), "player1", std::to_string(roomInfo.player1));
    redis_->hset(key.str(), "player2", std::to_string(roomInfo.player2));
    redis_->hset(key.str(), "current_turn", std::to_string(roomInfo.currentTurn));
    redis_->hset(key.str(), "start_time", std::to_string(roomInfo.startTime));
    
    // 保存观战者列表
    std::ostringstream spectatorsKey;
    spectatorsKey << "room:" << roomInfo.roomId << ":spectators";
    redis_->srem(spectatorsKey.str(), ""); // 清空
    for (int spectator : roomInfo.spectators) {
        redis_->sadd(spectatorsKey.str(), std::to_string(spectator));
    }
    
    return true;
}

bool RoomManager::removeRoomFromRedis(const std::string& roomId) {
    std::ostringstream key;
    key << "room:" << roomId;
    
    // 删除房间信息
    redis_->del(key.str());
    
    // 删除房间成员集合
    std::ostringstream membersKey;
    membersKey << "room:" << roomId << ":members";
    redis_->del(membersKey.str());
    
    // 删除观战者集合
    std::ostringstream spectatorsKey;
    spectatorsKey << "room:" << roomId << ":spectators";
    redis_->del(spectatorsKey.str());
    
    // 删除棋步记录
    std::ostringstream movesKey;
    movesKey << "room:" << roomId << ":moves";
    redis_->del(movesKey.str());
    
    return true;
}

bool RoomManager::validatePassword(const std::string& roomId, const std::string& password) {
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return false;
    }
    
    return info.password == password;
}

std::vector<int> RoomManager::getRoomUsers(const std::string& roomId) {
    std::vector<int> userIds;
    
    InternalRoomInfo info;
    if (!getRoomInfo(roomId, info)) {
        return userIds;
    }
    
    if (info.player1 != 0) {
        userIds.push_back(info.player1);
    }
    if (info.player2 != 0) {
        userIds.push_back(info.player2);
    }
    
    for (int spectator : info.spectators) {
        userIds.push_back(spectator);
    }
    
    return userIds;
}

} // namespace gomoku