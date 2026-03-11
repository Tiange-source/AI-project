#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "storage/MySQLClient.h"
#include "storage/RedisClient.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace gomoku {

struct UserInfo {
    int userId;
    std::string username;
    std::string nickname;
    std::string avatarUrl;
    int winCount;
    int loseCount;
    int drawCount;
    int rating;
    int totalGames;
};

class UserManager {
public:
    UserManager();
    ~UserManager();
    
    // 初始化
    bool initialize(MySQLClient* mysql, RedisClient* redis);
    
    // 用户登录
    int login(const std::string& username, const std::string& password, 
              UserInfo& userInfo, std::string& token);
    
    // 用户注册
    int registerUser(const std::string& username, const std::string& password,
                     const std::string& email, const std::string& nickname,
                     UserInfo& userInfo);
    
    // 用户登出
    bool logout(int userId, const std::string& token);
    
    // 添加在线用户
    void addOnlineUser(int userId, const UserInfo& info, const std::string& token);
    
    // 移除在线用户
    void removeOnlineUser(int userId);
    
    // 获取用户信息
    bool getUserInfo(int userId, UserInfo& info);
    
    // 更新用户信息
    bool updateUserInfo(const UserInfo& info);
    
    // 检查Token是否有效
    bool isTokenValid(int userId, const std::string& token);
    
    // 根据Token获取用户ID
    int getUserIdByToken(const std::string& token);
    
    // 更新战绩
    bool updateStats(int userId, bool isWin, int steps = 0);
    
    // 获取在线用户列表
    std::vector<UserInfo> getOnlineUsers();
    
    // 检查用户是否在线
    bool isUserOnline(int userId);
    
    // 验证密码（简单实现，实际应该使用bcrypt）
    bool validatePassword(const std::string& input, const std::string& stored);

private:
    // 生成Token
    std::string generateToken(int userId);
    
    // 从数据库加载用户信息
    bool loadUserFromDB(int userId, UserInfo& info);
    
    // 从数据库加载用户信息（通过用户名）
    bool loadUserFromDB(const std::string& username, UserInfo& info);
    
    // 保存用户信息到数据库
    bool saveUserToDB(const UserInfo& info);
    
    // 将用户信息缓存到Redis
    void cacheUserToRedis(int userId, const UserInfo& info);
    
    // 从Redis获取用户信息
    bool getUserFromRedis(int userId, UserInfo& info);

    MySQLClient* mysql_;
    RedisClient* redis_;
    std::mutex mutex_;
    
    // 在线用户映射（本地缓存）
    std::map<int, UserInfo> onlineUsers_;
    std::map<int, std::string> userTokens_;
};

} // namespace gomoku

#endif // USER_MANAGER_H