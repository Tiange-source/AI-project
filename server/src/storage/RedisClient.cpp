#include "storage/RedisClient.h"
#include <iostream>
#include <cstdarg>
#include <cstring>

namespace gomoku {

// ========================================
// RedisReply 实现
// ========================================
RedisReply::RedisReply(redisReply* reply) : reply_(reply) {
}

RedisReply::~RedisReply() {
    if (reply_) {
        freeReplyObject(reply_);
    }
}

bool RedisReply::isNull() const {
    return reply_ == nullptr;
}

bool RedisReply::isString() const {
    return reply_ && reply_->type == REDIS_REPLY_STRING;
}

bool RedisReply::isInteger() const {
    return reply_ && reply_->type == REDIS_REPLY_INTEGER;
}

bool RedisReply::isArray() const {
    return reply_ && reply_->type == REDIS_REPLY_ARRAY;
}

bool RedisReply::isStatus() const {
    return reply_ && reply_->type == REDIS_REPLY_STATUS;
}

bool RedisReply::isError() const {
    return reply_ && reply_->type == REDIS_REPLY_ERROR;
}

std::string RedisReply::getString() const {
    if (!isString() && !isStatus()) {
        return "";
    }
    return std::string(reply_->str, reply_->len);
}

long long RedisReply::getInteger() const {
    if (!isInteger()) {
        return 0;
    }
    return reply_->integer;
}

std::vector<std::string> RedisReply::getArray() const {
    std::vector<std::string> result;
    if (!isArray()) {
        return result;
    }
    
    for (size_t i = 0; i < reply_->elements; ++i) {
        redisReply* element = reply_->element[i];
        if (element->type == REDIS_REPLY_STRING) {
            result.push_back(std::string(element->str, element->len));
        } else if (element->type == REDIS_REPLY_INTEGER) {
            result.push_back(std::to_string(element->integer));
        }
    }
    
    return result;
}

std::string RedisReply::getError() const {
    if (!isError()) {
        return "";
    }
    return std::string(reply_->str, reply_->len);
}

// ========================================
// RedisClient 实现
// ========================================
RedisClient::RedisClient() : context_(nullptr), connected_(false) {
}

RedisClient::~RedisClient() {
    close();
}

bool RedisClient::initialize(const RedisConfig& config) {
    config_ = config;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 连接到Redis服务器
    struct timeval timeout = {1, 500000}; // 1.5秒超时
    context_ = redisConnectWithTimeout(config.host.c_str(), config.port, timeout);
    
    if (context_ == nullptr || context_->err) {
        if (context_) {
            std::cerr << "Redis connection error: " << context_->errstr << std::endl;
            redisFree(context_);
            context_ = nullptr;
        } else {
            std::cerr << "Redis connection error: can't allocate redis context" << std::endl;
        }
        return false;
    }
    
    // 认证
    if (!config.password.empty()) {
        auto reply = executeCommand("AUTH %s", config.password.c_str());
        if (reply.isError() || reply.isNull()) {
            std::cerr << "Redis authentication failed" << std::endl;
            close();
            return false;
        }
    }
    
    // 选择数据库
    if (config.dbIndex != 0) {
        auto reply = executeCommand("SELECT %d", config.dbIndex);
        if (reply.isError() || reply.isNull()) {
            std::cerr << "Redis select database failed" << std::endl;
            close();
            return false;
        }
    }
    
    connected_ = true;
    std::cout << "Redis connected to " << config.host << ":" << config.port << std::endl;
    return true;
}

void RedisClient::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
    connected_ = false;
}

bool RedisClient::isConnected() const {
    return connected_ && context_ != nullptr;
}

bool RedisClient::reconnect() {
    close();
    return initialize(config_);
}

RedisReply RedisClient::executeCommand(const char* format, ...) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isConnected()) {
        return RedisReply(nullptr);
    }
    
    va_list args;
    va_start(args, format);
    redisReply* reply = (redisReply*)redisvCommand(context_, format, args);
    va_end(args);
    
    if (reply == nullptr) {
        std::cerr << "Redis command failed: " << context_->errstr << std::endl;
        // 连接断开，尝试重连
        if (context_->err == REDIS_ERR_EOF || context_->err == REDIS_ERR_IO) {
            connected_ = false;
        }
        return RedisReply(nullptr);
    }
    
    return RedisReply(reply);
}

// ========================================
// 基本操作
// ========================================
bool RedisClient::set(const std::string& key, const std::string& value, int expireSeconds) {
    if (expireSeconds > 0) {
        auto reply = executeCommand("SETEX %s %d %s", key.c_str(), expireSeconds, value.c_str());
        return !reply.isError() && !reply.isNull();
    } else {
        auto reply = executeCommand("SET %s %s", key.c_str(), value.c_str());
        return !reply.isError() && !reply.isNull();
    }
}

std::string RedisClient::get(const std::string& key) {
    auto reply = executeCommand("GET %s", key.c_str());
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

bool RedisClient::del(const std::string& key) {
    auto reply = executeCommand("DEL %s", key.c_str());
    return !reply.isError() && !reply.isNull();
}

bool RedisClient::del(const std::vector<std::string>& keys) {
    if (keys.empty()) {
        return false;
    }
    
    std::string cmd = "DEL";
    for (const auto& key : keys) {
        cmd += " " + key;
    }
    
    auto reply = executeCommand(cmd.c_str());
    return !reply.isError() && !reply.isNull();
}

bool RedisClient::exists(const std::string& key) {
    auto reply = executeCommand("EXISTS %s", key.c_str());
    if (reply.isInteger()) {
        return reply.getInteger() > 0;
    }
    return false;
}

bool RedisClient::expire(const std::string& key, int seconds) {
    auto reply = executeCommand("EXPIRE %s %d", key.c_str(), seconds);
    return !reply.isError() && !reply.isNull();
}

int RedisClient::ttl(const std::string& key) {
    auto reply = executeCommand("TTL %s", key.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return -1;
}

// ========================================
// 哈希表操作
// ========================================
bool RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto reply = executeCommand("HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    return !reply.isError() && !reply.isNull();
}

std::string RedisClient::hget(const std::string& key, const std::string& field) {
    auto reply = executeCommand("HGET %s %s", key.c_str(), field.c_str());
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

std::vector<std::pair<std::string, std::string>> RedisClient::hgetAll(const std::string& key) {
    std::vector<std::pair<std::string, std::string>> result;
    auto reply = executeCommand("HGETALL %s", key.c_str());
    
    if (reply.isArray()) {
        auto array = reply.getArray();
        for (size_t i = 0; i < array.size(); i += 2) {
            if (i + 1 < array.size()) {
                result.push_back({array[i], array[i + 1]});
            }
        }
    }
    
    return result;
}

bool RedisClient::hdel(const std::string& key, const std::string& field) {
    auto reply = executeCommand("HDEL %s %s", key.c_str(), field.c_str());
    return !reply.isError() && !reply.isNull();
}

bool RedisClient::hexists(const std::string& key, const std::string& field) {
    auto reply = executeCommand("HEXISTS %s %s", key.c_str(), field.c_str());
    if (reply.isInteger()) {
        return reply.getInteger() > 0;
    }
    return false;
}

std::vector<std::string> RedisClient::hkeys(const std::string& key) {
    auto reply = executeCommand("HKEYS %s", key.c_str());
    if (reply.isArray()) {
        return reply.getArray();
    }
    return {};
}

std::vector<std::string> RedisClient::hvals(const std::string& key) {
    auto reply = executeCommand("HVALS %s", key.c_str());
    if (reply.isArray()) {
        return reply.getArray();
    }
    return {};
}

int RedisClient::hlen(const std::string& key) {
    auto reply = executeCommand("HLEN %s", key.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

// ========================================
// 有序集合操作
// ========================================
bool RedisClient::zadd(const std::string& key, double score, const std::string& member) {
    auto reply = executeCommand("ZADD %s %f %s", key.c_str(), score, member.c_str());
    return !reply.isError() && !reply.isNull();
}

std::vector<std::string> RedisClient::zrange(const std::string& key, int start, int stop, bool withScores) {
    if (withScores) {
        auto reply = executeCommand("ZRANGE %s %d %d WITHSCORES", key.c_str(), start, stop);
        if (reply.isArray()) {
            return reply.getArray();
        }
    } else {
        auto reply = executeCommand("ZRANGE %s %d %d", key.c_str(), start, stop);
        if (reply.isArray()) {
            return reply.getArray();
        }
    }
    return {};
}

std::vector<std::string> RedisClient::zrevrange(const std::string& key, int start, int stop, bool withScores) {
    if (withScores) {
        auto reply = executeCommand("ZREVRANGE %s %d %d WITHSCORES", key.c_str(), start, stop);
        if (reply.isArray()) {
            return reply.getArray();
        }
    } else {
        auto reply = executeCommand("ZREVRANGE %s %d %d", key.c_str(), start, stop);
        if (reply.isArray()) {
            return reply.getArray();
        }
    }
    return {};
}

int RedisClient::zremrangebyrank(const std::string& key, int start, int stop) {
    auto reply = executeCommand("ZREMRANGEBYRANK %s %d %d", key.c_str(), start, stop);
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

bool RedisClient::zrem(const std::string& key, const std::string& member) {
    auto reply = executeCommand("ZREM %s %s", key.c_str(), member.c_str());
    return !reply.isError() && !reply.isNull();
}

int RedisClient::zcard(const std::string& key) {
    auto reply = executeCommand("ZCARD %s", key.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

double RedisClient::zscore(const std::string& key, const std::string& member) {
    auto reply = executeCommand("ZSCORE %s %s", key.c_str(), member.c_str());
    if (reply.isString()) {
        return std::stod(reply.getString());
    }
    return 0.0;
}

int RedisClient::zrank(const std::string& key, const std::string& member) {
    auto reply = executeCommand("ZRANK %s %s", key.c_str(), member.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return -1;
}

// ========================================
// 列表操作
// ========================================
int RedisClient::lpush(const std::string& key, const std::string& value) {
    auto reply = executeCommand("LPUSH %s %s", key.c_str(), value.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

int RedisClient::rpush(const std::string& key, const std::string& value) {
    auto reply = executeCommand("RPUSH %s %s", key.c_str(), value.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

std::string RedisClient::lpop(const std::string& key) {
    auto reply = executeCommand("LPOP %s", key.c_str());
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

std::string RedisClient::rpop(const std::string& key) {
    auto reply = executeCommand("RPOP %s", key.c_str());
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

std::vector<std::string> RedisClient::lrange(const std::string& key, int start, int stop) {
    auto reply = executeCommand("LRANGE %s %d %d", key.c_str(), start, stop);
    if (reply.isArray()) {
        return reply.getArray();
    }
    return {};
}

int RedisClient::llen(const std::string& key) {
    auto reply = executeCommand("LLEN %s", key.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

std::string RedisClient::lindex(const std::string& key, int index) {
    auto reply = executeCommand("LINDEX %s %d", key.c_str(), index);
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

// ========================================
// 集合操作
// ========================================
int RedisClient::sadd(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SADD %s %s", key.c_str(), member.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

int RedisClient::srem(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SREM %s %s", key.c_str(), member.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

std::vector<std::string> RedisClient::smembers(const std::string& key) {
    auto reply = executeCommand("SMEMBERS %s", key.c_str());
    if (reply.isArray()) {
        return reply.getArray();
    }
    return {};
}

bool RedisClient::sismember(const std::string& key, const std::string& member) {
    auto reply = executeCommand("SISMEMBER %s %s", key.c_str(), member.c_str());
    if (reply.isInteger()) {
        return reply.getInteger() > 0;
    }
    return false;
}

int RedisClient::scard(const std::string& key) {
    auto reply = executeCommand("SCARD %s", key.c_str());
    if (reply.isInteger()) {
        return static_cast<int>(reply.getInteger());
    }
    return 0;
}

std::string RedisClient::spop(const std::string& key) {
    auto reply = executeCommand("SPOP %s", key.c_str());
    if (reply.isString()) {
        return reply.getString();
    }
    return "";
}

} // namespace gomoku