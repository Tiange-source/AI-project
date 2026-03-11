#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H

#include <hiredis/hiredis.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>

namespace gomoku {

// Redis配置
struct RedisConfig {
    std::string host = "localhost";
    int port = 6379;
    std::string password = "";
    int dbIndex = 0;
};

// Redis回复类
class RedisReply {
public:
    RedisReply(redisReply* reply);
    ~RedisReply();

    bool isNull() const;
    bool isString() const;
    bool isInteger() const;
    bool isArray() const;
    bool isStatus() const;
    bool isError() const;
    
    std::string getString() const;
    long long getInteger() const;
    std::vector<std::string> getArray() const;
    std::string getError() const;

private:
    redisReply* reply_;
};

// Redis客户端类
class RedisClient {
public:
    RedisClient();
    ~RedisClient();

    // 初始化客户端
    bool initialize(const RedisConfig& config);
    
    // 关闭客户端
    void close();
    
    // 连接状态
    bool isConnected() const;
    
    // 重新连接
    bool reconnect();

    // ========================================
    // 基本操作
    // ========================================
    
    // SET key value [EX seconds]
    bool set(const std::string& key, const std::string& value, int expireSeconds = 0);
    
    // GET key
    std::string get(const std::string& key);
    
    // DEL key [key ...]
    bool del(const std::string& key);
    bool del(const std::vector<std::string>& keys);
    
    // EXISTS key
    bool exists(const std::string& key);
    
    // EXPIRE key seconds
    bool expire(const std::string& key, int seconds);
    
    // TTL key
    int ttl(const std::string& key);

    // ========================================
    // 哈希表操作
    // ========================================
    
    // HSET key field value
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    
    // HGET key field
    std::string hget(const std::string& key, const std::string& field);
    
    // HGETALL key
    std::vector<std::pair<std::string, std::string>> hgetAll(const std::string& key);
    
    // HDEL key field [field ...]
    bool hdel(const std::string& key, const std::string& field);
    
    // HEXISTS key field
    bool hexists(const std::string& key, const std::string& field);
    
    // HKEYS key
    std::vector<std::string> hkeys(const std::string& key);
    
    // HVALS key
    std::vector<std::string> hvals(const std::string& key);
    
    // HLEN key
    int hlen(const std::string& key);

    // ========================================
    // 有序集合操作
    // ========================================
    
    // ZADD key score member [score member ...]
    bool zadd(const std::string& key, double score, const std::string& member);
    
    // ZRANGE key start stop [WITHSCORES]
    std::vector<std::string> zrange(const std::string& key, int start, int stop, bool withScores = false);
    
    // ZREVRANGE key start stop [WITHSCORES]
    std::vector<std::string> zrevrange(const std::string& key, int start, int stop, bool withScores = false);
    
    // ZREMRANGEBYRANK key start stop
    int zremrangebyrank(const std::string& key, int start, int stop);
    
    // ZREM key member [member ...]
    bool zrem(const std::string& key, const std::string& member);
    
    // ZCARD key
    int zcard(const std::string& key);
    
    // ZSCORE key member
    double zscore(const std::string& key, const std::string& member);
    
    // ZRANK key member
    int zrank(const std::string& key, const std::string& member);

    // ========================================
    // 列表操作
    // ========================================
    
    // LPUSH key value [value ...]
    int lpush(const std::string& key, const std::string& value);
    
    // RPUSH key value [value ...]
    int rpush(const std::string& key, const std::string& value);
    
    // LPOP key
    std::string lpop(const std::string& key);
    
    // RPOP key
    std::string rpop(const std::string& key);
    
    // LRANGE key start stop
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    
    // LLEN key
    int llen(const std::string& key);
    
    // LINDEX key index
    std::string lindex(const std::string& key, int index);

    // ========================================
    // 集合操作
    // ========================================
    
    // SADD key member [member ...]
    int sadd(const std::string& key, const std::string& member);
    
    // SREM key member [member ...]
    int srem(const std::string& key, const std::string& member);
    
    // SMEMBERS key
    std::vector<std::string> smembers(const std::string& key);
    
    // SISMEMBER key member
    bool sismember(const std::string& key, const std::string& member);
    
    // SCARD key
    int scard(const std::string& key);
    
    // SPOP key
    std::string spop(const std::string& key);

private:
    // 执行Redis命令
    RedisReply executeCommand(const char* format, ...);
    
    redisContext* context_;
    RedisConfig config_;
    std::mutex mutex_;
    bool connected_;
};

} // namespace gomoku

#endif // REDIS_CLIENT_H