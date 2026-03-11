#ifndef SPECTATOR_MANAGER_H
#define SPECTATOR_MANAGER_H

#include "storage/RedisClient.h"
#include "business/UserManager.h"
#include "business/RoomManager.h"
#include <string>
#include <vector>
#include <mutex>

namespace gomoku {

class SpectatorManager {
public:
    SpectatorManager();
    ~SpectatorManager();
    
    // 初始化
    bool initialize(RedisClient* redis, UserManager* userManager, RoomManager* roomManager);
    
    // 加入观战
    bool addSpectator(const std::string& roomId, int userId);
    
    // 离开观战
    bool removeSpectator(const std::string& roomId, int userId);
    
    // 获取观战者列表
    std::vector<int> getSpectators(const std::string& roomId);
    
    // 获取历史棋步
    std::vector<std::string> getMoveHistory(const std::string& roomId);

private:
    RedisClient* redis_;
    UserManager* userManager_;
    RoomManager* roomManager_;
    std::mutex mutex_;
};

} // namespace gomoku

#endif // SPECTATOR_MANAGER_H