#include "business/UserManager.h"
#include "utils/Logger.h"
#include <sstream>
#include <random>
#include <ctime>
#include <cstring>

namespace gomoku {

UserManager::UserManager() : mysql_(nullptr), redis_(nullptr) {
}

UserManager::~UserManager() {
}

bool UserManager::initialize(MySQLClient* mysql, RedisClient* redis) {
    mysql_ = mysql;
    redis_ = redis;
    
    if (!mysql_ || !redis_) {
        LOG_ERROR("UserManager::initialize - mysql or redis is null");
        return false;
    }
    
    LOG_INFO("UserManager initialized");
    return true;
}

int UserManager::login(const std::string& username, const std::string& password, 
                       UserInfo& userInfo, std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从数据库查询用户
    UserInfo info;
    if (!loadUserFromDB(username, info)) {
        LOG_WARN("UserManager::login - user not found: " + username);
        return -1;
    }
    
    // 验证密码
    // 注意：实际项目中应该使用bcrypt等安全哈希算法
    // 这里简化处理
    if (!validatePassword(password, "stored_password_hash")) {
        LOG_WARN("UserManager::login - password incorrect for user: " + username);
        return -1;
    }
    
    // 生成Token
    token = generateToken(info.userId);
    
    // 保存到在线用户
    addOnlineUser(info.userId, info, token);
    
    // 更新最后登录时间
    std::ostringstream sql;
    sql << "UPDATE users SET last_login = NOW() WHERE user_id = " << info.userId;
    mysql_->execute(sql.str());
    
    userInfo = info;
    
    LOG_INFO("UserManager::login - user logged in: " + username + " (ID: " + std::to_string(info.userId) + ")");
    return info.userId;
}

int UserManager::registerUser(const std::string& username, const std::string& password,
                              const std::string& email, const std::string& nickname,
                              UserInfo& userInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查用户名是否已存在
    UserInfo existingUser;
    if (loadUserFromDB(username, existingUser)) {
        LOG_WARN("UserManager::registerUser - username already exists: " + username);
        return -1;
    }
    
    // 插入新用户到数据库
    std::ostringstream sql;
    sql << "INSERT INTO users (username, password_hash, email, nickname, avatar_url, "
        << "win_count, lose_count, draw_count, rating, total_games) VALUES ('"
        << username << "', '" << password << "', '" << email << "', '" << nickname
        << "', '', 0, 0, 0, 1000, 0)";
    
    if (mysql_->execute(sql.str()) <= 0) {
        LOG_ERROR("UserManager::registerUser - failed to insert user");
        return -1;
    }
    
    // 获取新用户的ID
    int userId = mysql_->getLastInsertId();
    
    // 加载用户信息
    loadUserFromDB(userId, userInfo);
    
    LOG_INFO("UserManager::registerUser - user registered: " + username + " (ID: " + std::to_string(userId) + ")");
    return userId;
}

bool UserManager::logout(int userId, const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 验证Token
    if (!isTokenValid(userId, token)) {
        LOG_WARN("UserManager::logout - invalid token for user ID: " + std::to_string(userId));
        return false;
    }
    
    // 从在线用户中移除
    removeOnlineUser(userId);
    
    LOG_INFO("UserManager::logout - user logged out: ID " + std::to_string(userId));
    return true;
}

void UserManager::addOnlineUser(int userId, const UserInfo& info, const std::string& token) {
    onlineUsers_[userId] = info;
    userTokens_[userId] = token;
    
    // 缓存到Redis
    cacheUserToRedis(userId, info);
    
    // 添加到在线用户集合
    redis_->sadd("online_users", std::to_string(userId));
    
    // 保存Token映射
    std::ostringstream key;
    key << "user:token:" << token;
    redis_->set(key.str(), std::to_string(userId), 86400); // 24小时过期
}

void UserManager::removeOnlineUser(int userId) {
    onlineUsers_.erase(userId);
    userTokens_.erase(userId);
    
    // 从Redis移除
    redis_->srem("online_users", std::to_string(userId));
}

bool UserManager::getUserInfo(int userId, UserInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 先从本地缓存查找
    auto it = onlineUsers_.find(userId);
    if (it != onlineUsers_.end()) {
        info = it->second;
        return true;
    }
    
    // 从Redis查找
    if (getUserFromRedis(userId, info)) {
        return true;
    }
    
    // 从数据库查找
    return loadUserFromDB(userId, info);
}

bool UserManager::updateUserInfo(const UserInfo& info) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 更新数据库
    if (!saveUserToDB(info)) {
        return false;
    }
    
    // 更新缓存
    onlineUsers_[info.userId] = info;
    cacheUserToRedis(info.userId, info);
    
    return true;
}

bool UserManager::isTokenValid(int userId, const std::string& token) {
    auto it = userTokens_.find(userId);
    if (it != userTokens_.end()) {
        return it->second == token;
    }
    
    // 检查Redis
    std::ostringstream key;
    key << "user:token:" << token;
    std::string userIdStr = redis_->get(key.str());
    if (!userIdStr.empty()) {
        return std::stoi(userIdStr) == userId;
    }
    
    return false;
}

int UserManager::getUserIdByToken(const std::string& token) {
    std::ostringstream key;
    key << "user:token:" << token;
    std::string userIdStr = redis_->get(key.str());
    
    if (!userIdStr.empty()) {
        return std::stoi(userIdStr);
    }
    
    return -1;
}

bool UserManager::updateStats(int userId, bool isWin, int steps) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    UserInfo info;
    if (!getUserInfo(userId, info)) {
        return false;
    }
    
    // 更新战绩
    if (isWin) {
        info.winCount++;
        info.rating += 25;
        if (steps > 0 && (info.winCount == 1 || steps < info.totalGames)) {
            // 记录最少步数
        }
    } else {
        info.loseCount++;
        info.rating = std::max(0, info.rating - 10);
    }
    
    info.totalGames++;
    
    // 更新数据库
    std::ostringstream sql;
    sql << "UPDATE users SET win_count = " << info.winCount
        << ", lose_count = " << info.loseCount
        << ", total_games = " << info.totalGames
        << ", rating = " << info.rating
        << " WHERE user_id = " << userId;
    
    return mysql_->execute(sql.str()) > 0;
}

std::vector<UserInfo> UserManager::getOnlineUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<UserInfo> users;
    for (const auto& pair : onlineUsers_) {
        users.push_back(pair.second);
    }
    
    return users;
}

bool UserManager::isUserOnline(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return onlineUsers_.find(userId) != onlineUsers_.end();
}

bool UserManager::validatePassword(const std::string& input, const std::string& stored) {
    // 简化实现：实际应该使用bcrypt
    // 这里只做示例
    return !input.empty();
}

std::string UserManager::generateToken(int userId) {
    // 生成随机Token
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex << userId << "_";
    
    for (int i = 0; i < 32; ++i) {
        ss << std::setw(1) << dis(gen);
    }
    
    return ss.str();
}

bool UserManager::loadUserFromDB(int userId, UserInfo& info) {
    std::ostringstream sql;
    sql << "SELECT user_id, username, nickname, avatar_url, win_count, lose_count, "
        << "draw_count, rating, total_games FROM users WHERE user_id = " << userId;
    
    auto result = mysql_->query(sql.str());
    if (!result || result->getRowCount() == 0) {
        return false;
    }
    
    if (result->next()) {
        info.userId = result->getInt(0);
        info.username = result->getString(1);
        info.nickname = result->getString(2);
        info.avatarUrl = result->getString(3);
        info.winCount = result->getInt(4);
        info.loseCount = result->getInt(5);
        info.drawCount = result->getInt(6);
        info.rating = result->getInt(7);
        info.totalGames = result->getInt(8);
        return true;
    }
    
    return false;
}

bool UserManager::loadUserFromDB(const std::string& username, UserInfo& info) {
    std::ostringstream sql;
    sql << "SELECT user_id, username, nickname, avatar_url, win_count, lose_count, "
        << "draw_count, rating, total_games FROM users WHERE username = '" << username << "'";
    
    auto result = mysql_->query(sql.str());
    if (!result || result->getRowCount() == 0) {
        return false;
    }
    
    if (result->next()) {
        info.userId = result->getInt(0);
        info.username = result->getString(1);
        info.nickname = result->getString(2);
        info.avatarUrl = result->getString(3);
        info.winCount = result->getInt(4);
        info.loseCount = result->getInt(5);
        info.drawCount = result->getInt(6);
        info.rating = result->getInt(7);
        info.totalGames = result->getInt(8);
        return true;
    }
    
    return false;
}

bool UserManager::saveUserToDB(const UserInfo& info) {
    std::ostringstream sql;
    sql << "UPDATE users SET nickname = '" << info.nickname
        << "', avatar_url = '" << info.avatarUrl
        << "', win_count = " << info.winCount
        << ", lose_count = " << info.loseCount
        << ", draw_count = " << info.drawCount
        << ", rating = " << info.rating
        << ", total_games = " << info.totalGames
        << " WHERE user_id = " << info.userId;
    
    return mysql_->execute(sql.str()) > 0;
}

void UserManager::cacheUserToRedis(int userId, const UserInfo& info) {
    if (!redis_) return;
    
    std::ostringstream key;
    key << "user:" << userId;
    
    // 使用Hash存储用户信息
    redis_->hset(key.str(), "user_id", std::to_string(info.userId));
    redis_->hset(key.str(), "username", info.username);
    redis_->hset(key.str(), "nickname", info.nickname);
    redis_->hset(key.str(), "avatar_url", info.avatarUrl);
    redis_->hset(key.str(), "win_count", std::to_string(info.winCount));
    redis_->hset(key.str(), "lose_count", std::to_string(info.loseCount));
    redis_->hset(key.str(), "draw_count", std::to_string(info.drawCount));
    redis_->hset(key.str(), "rating", std::to_string(info.rating));
    redis_->hset(key.str(), "total_games", std::to_string(info.totalGames));
    
    // 设置过期时间
    redis_->expire(key.str(), 3600); // 1小时
}

bool UserManager::getUserFromRedis(int userId, UserInfo& info) {
    if (!redis_) return false;
    
    std::ostringstream key;
    key << "user:" << userId;
    
    auto fields = redis_->hgetAll(key.str());
    if (fields.empty()) {
        return false;
    }
    
    for (const auto& pair : fields) {
        if (pair.first == "user_id") {
            info.userId = std::stoi(pair.second);
        } else if (pair.first == "username") {
            info.username = pair.second;
        } else if (pair.first == "nickname") {
            info.nickname = pair.second;
        } else if (pair.first == "avatar_url") {
            info.avatarUrl = pair.second;
        } else if (pair.first == "win_count") {
            info.winCount = std::stoi(pair.second);
        } else if (pair.first == "lose_count") {
            info.loseCount = std::stoi(pair.second);
        } else if (pair.first == "draw_count") {
            info.drawCount = std::stoi(pair.second);
        } else if (pair.first == "rating") {
            info.rating = std::stoi(pair.second);
        } else if (pair.first == "total_games") {
            info.totalGames = std::stoi(pair.second);
        }
    }
    
    return true;
}

} // namespace gomoku