#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "storage/RedisClient.h"
#include "business/UserManager.h"
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <functional>

namespace gomoku {

enum class RoomState {
    WAITING,
    GAMING,
    FINISHED
};

struct RoomInfo {
    std::string roomId;
    std::string roomName;
    int ownerId;
    std::string password;
    bool hasPassword;
    RoomState state;
    int player1;
    int player2;
    std::vector<int> spectators;
    int currentTurn;
    long long startTime;
};

class RoomManager {
public:
    using BroadcastCallback = std::function<void(const std::vector<int>& userIds, const std::string& message)>;
    
    RoomManager();
    ~RoomManager();
    
    // 初始化
    bool initialize(RedisClient* redis, UserManager* userManager);
    
    // 设置广播回调
    void setBroadcastCallback(const BroadcastCallback& cb);
    
    // 创建房间
    bool createRoom(int ownerId, const std::string& roomName, const std::string& password, 
                    RoomInfo& roomInfo);
    
    // 加入房间
    bool joinRoom(int userId, const std::string& roomId, const std::string& password, 
                  RoomInfo& roomInfo);
    
    // 离开房间
    bool leaveRoom(int userId, const std::string& roomId);
    
    // 关闭房间
    bool closeRoom(const std::string& roomId);
    
    // 获取房间信息
    bool getRoomInfo(const std::string& roomId, RoomInfo& roomInfo);
    
    // 获取活跃房间列表
    std::vector<RoomInfo> getActiveRooms();
    
    // 检查房间是否已满
    bool isRoomFull(const std::string& roomId);
    
    // 检查房间是否在游戏中
    bool isRoomGaming(const std::string& roomId);
    
    // 开始游戏
    bool startGame(const std::string& roomId);
    
    // 更新房间状态
    bool updateRoomState(const std::string& roomId, RoomState state);
    
    // 添加观战者
    bool addSpectator(const std::string& roomId, int userId);
    
    // 移除观战者
    bool removeSpectator(const std::string& roomId, int userId);
    
    // 获取观战者列表
    std::vector<int> getSpectators(const std::string& roomId);
    
    // 广播消息到房间
    bool broadcastToRoom(const std::string& roomId, const std::string& message);
    
    // 生成房间号
    std::string generateRoomId();

private:
    // 从Redis加载房间信息
    bool loadRoomFromRedis(const std::string& roomId, RoomInfo& roomInfo);
    
    // 保存房间信息到Redis
    bool saveRoomToRedis(const RoomInfo& roomInfo);
    
    // 从Redis移除房间
    bool removeRoomFromRedis(const std::string& roomId);
    
    // 验证房间密码
    bool validatePassword(const std::string& roomId, const std::string& password);
    
    // 获取房间内的所有用户ID
    std::vector<int> getRoomUsers(const std::string& roomId);

    RedisClient* redis_;
    UserManager* userManager_;
    std::mutex mutex_;
    BroadcastCallback broadcastCallback_;
    
    // 本地房间缓存
    std::map<std::string, RoomInfo> rooms_;
};

} // namespace gomoku

#endif // ROOM_MANAGER_H