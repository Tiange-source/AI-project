#include "business/UserManager.h"
#include "utils/Logger.h"
#include <sstream>
#include <random>
#include <ctime>
#include <cstring>
#include <iomanip>

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
                       InternalUserInfo& userInfo, std::string& token) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 从数据库查询用户
    InternalUserInfo info;
    if (!loadUserFromDB(username, info)) {
        LOG_WARN("UserManager::login - user not found: " + username);
        return -1;
    }

    // 验证密码
    // 注意：实际项目中应该使用bcrypt等安全哈希算法
    // 这里简化处理，使用从数据库加载的密码哈希
    if (!validatePassword(password, info.passwordHash)) {
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
                              InternalUserInfo& userInfo) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查用户名是否已存在
    InternalUserInfo existingUser;
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

bool UserManager::logout(int userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 从在线用户中移除（不验证token）
    removeOnlineUser(userId);
    
    LOG_INFO("UserManager::logout - user logged out (connection closed): ID " + std::to_string(userId));
    return true;
}

void UserManager::addOnlineUser(int userId, const InternalUserInfo& info, const std::string& token) {
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

bool UserManager::getUserInfo(int userId, InternalUserInfo& info) {
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

bool UserManager::updateUserInfo(const InternalUserInfo& info) {
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
    
    InternalUserInfo info;
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

std::vector<InternalUserInfo> UserManager::getOnlineUsers() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<InternalUserInfo> users;
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
    // 简化实现：实际应该使用bcrypt等安全哈希算法
    // 这里简化处理，直接比较明文密码和存储的密码哈希
    // 注意：这只是示例，实际项目中不应该使用明文密码存储
    return input == stored;
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

bool UserManager::loadUserFromDB(int userId, InternalUserInfo& info) {
    std::ostringstream sql;
    sql << "SELECT user_id, username, nickname, avatar_url, password_hash, win_count, lose_count, "
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
        info.passwordHash = result->getString(4);
        info.winCount = result->getInt(5);
        info.loseCount = result->getInt(6);
        info.drawCount = result->getInt(7);
        info.rating = result->getInt(8);
        info.totalGames = result->getInt(9);
        return true;
    }
    
    return false;
}

bool UserManager::loadUserFromDB(const std::string& username, InternalUserInfo& info) {
    std::ostringstream sql;
    sql << "SELECT user_id, username, nickname, avatar_url, password_hash, win_count, lose_count, "
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
        info.passwordHash = result->getString(4);
        info.winCount = result->getInt(5);
        info.loseCount = result->getInt(6);
        info.drawCount = result->getInt(7);
        info.rating = result->getInt(8);
        info.totalGames = result->getInt(9);
        return true;
    }
    
    return false;
}

bool UserManager::saveUserToDB(const InternalUserInfo& info) {
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

void UserManager::cacheUserToRedis(int userId, const InternalUserInfo& info) {
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

bool UserManager::getUserFromRedis(int userId, InternalUserInfo& info) {
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


std::vector<InternalUserInfo> UserManager::getRankList(int type, int limit) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<InternalUserInfo> result;
    std::ostringstream sql;
    
    switch (type) {
        case 0: // 胜场数
            sql << "SELECT * FROM users ORDER BY win_count DESC LIMIT " << limit;
            break;
        case 1: // 积分
            sql << "SELECT * FROM users ORDER BY rating DESC LIMIT " << limit;
            break;
        case 2: // 胜率
            sql << "SELECT * FROM users ORDER BY (win_count * 100.0 / NULLIF(total_games, 0)) DESC LIMIT " << limit;
            break;
        default:
            sql << "SELECT * FROM users ORDER BY rating DESC LIMIT " << limit;
            break;
    }
    
    auto mysqlResult = mysql_->query(sql.str());
    if (!mysqlResult) {
        LOG_ERROR("UserManager::getRankList - failed to query database");
        return result;
    }
    
    while (mysqlResult->next()) {
        InternalUserInfo info;
        info.userId = mysqlResult->getInt("user_id");
        info.username = mysqlResult->getString("username");
        info.nickname = mysqlResult->getString("nickname");
        info.winCount = mysqlResult->getInt("win_count");
        info.loseCount = mysqlResult->getInt("lose_count");
        info.drawCount = mysqlResult->getInt("draw_count");
        info.rating = mysqlResult->getInt("rating");
        info.totalGames = mysqlResult->getInt("total_games");
        result.push_back(info);
    }
    
    return result;
}

} // namespace gomoku