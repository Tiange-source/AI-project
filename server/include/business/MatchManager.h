#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include "storage/RedisClient.h"
#include "business/UserManager.h"
#include "business/RoomManager.h"
#include <string>
#include <queue>
#include <mutex>

namespace gomoku {

class MatchManager {
public:
    using BroadcastCallback = std::function<void(int userId, const std::string& message)>;
    
    MatchManager();
    ~MatchManager();
    
    // 初始化
    bool initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager);
    
    // 设置广播回调
    void setBroadcastCallback(const BroadcastCallback& cb);
    
    // 请求匹配
    bool requestMatch(int userId);
    
    // 取消匹配
    bool cancelMatch(int userId);
    
    // 获取队列位置
    int getQueuePosition(int userId);
    
    // 尝试匹配
    bool tryMatch();

private:
    // 创建匹配房间
    bool createMatchRoom(int user1, int user2, std::string& roomId);
    
    // 通知匹配成功
    void notifyMatchSuccess(int user1, int user2, const std::string& roomId);
    
    // 从Redis加载匹配队列
    void loadMatchQueueFromRedis();

    RedisClient* redis_;
    UserManager* userManager_;
    RoomManager* roomManager_;
    std::mutex mutex_;
    BroadcastCallback broadcastCallback_;
    
    // 本地匹配队列
    std::queue<int> matchQueue_;
};

} // namespace gomoku

#endif // MATCH_MANAGER_H