#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include "network/EventLoop.h"
#include "network/Acceptor.h"
#include "network/TcpConnection.h"
#include "network/Buffer.h"
#include "storage/MySQLClient.h"
#include "storage/RedisClient.h"
#include "business/UserManager.h"
#include "business/RoomManager.h"
#include "business/GameController.h"
#include "business/MatchManager.h"
#include "business/ChatManager.h"
#include "business/SpectatorManager.h"
#include "protocol/ProtobufCodec.h"
#include "gomoku.pb.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace gomoku {

class GameServer {
public:
    GameServer();
    ~GameServer();
    
    // 初始化服务器
    bool initialize();
    
    // 启动服务器
    bool start();
    
    // 停止服务器
    void stop();
    
    // 加载配置
    bool loadConfig(const std::string& configFile);
    
    // 获取连接数
    int getConnectionCount() const { return connections_.size(); }

private:
    // 新连接回调
    void onNewConnection(int sockfd, const std::string& peerIp, uint16_t peerPort);
    
    // 连接关闭回调
    void onConnectionClosed(const TcpConnectionPtr& conn);
    
    // 消息接收回调
    void onMessage(const TcpConnectionPtr& conn, Buffer* buffer);
    
    // 消息路由
    void routeMessage(int userId, const ProtobufMessagePtr& message);
    
    // 登录处理
    void handleLogin(int connId, const std::string& username, const std::string& password);
    
    // 注册处理
    void handleRegister(int connId, const std::string& username, const std::string& password,
                       const std::string& email, const std::string& nickname);
    
    // 创建房间处理
    void handleCreateRoom(int userId, const std::string& roomName, const std::string& password);
    
    // 加入房间处理
    void handleJoinRoom(int userId, const std::string& roomId, const std::string& password);
    
    // 离开房间处理
    void handleLeaveRoom(int userId);
    
    // 开始游戏处理
    void handleStartGame(int userId);
    
    // 落子处理
    void handleMove(int userId, int row, int col);
    
    // 聊天消息处理
    void handleChatMessage(int userId, ChatType type, int targetId, const std::string& roomId, const std::string& content);
    
    // 观战处理
    void handleSpectate(int userId, const std::string& roomId);
    
    // 匹配处理
    void handleRandomMatch(int userId);
    
    // 排行榜处理
    void handleRankList(int userId, RankType type, int offset, int limit);
    
    // 广播消息
    void broadcast(int userId, const std::string& message);
    void broadcastToRoom(const std::string& roomId, const std::string& message);
    void broadcastToAll(const std::string& message);

private:
    std::string configFile_;
    int port_;
    std::string mysqlHost_;
    int mysqlPort_;
    std::string mysqlUser_;
    std::string mysqlPassword_;
    std::string mysqlDatabase_;
    std::string redisHost_;
    int redisPort_;
    
    // 网络组件
    EventLoop eventLoop_;
    std::unique_ptr<Acceptor> acceptor_;
    
    // 连接管理
    std::unordered_map<int, TcpConnectionPtr> connections_;  // connId -> connection
    std::unordered_map<int, int> connIdToUserId_;             // connId -> userId
    std::unordered_map<int, int> userIdToConnId_;             // userId -> connId
    
    // 存储层
    std::unique_ptr<MySQLClient> mysqlClient_;
    std::unique_ptr<RedisClient> redisClient_;
    
    // 业务层
    std::unique_ptr<UserManager> userManager_;
    std::unique_ptr<RoomManager> roomManager_;
    std::unique_ptr<GameController> gameController_;
    std::unique_ptr<MatchManager> matchManager_;
    std::unique_ptr<ChatManager> chatManager_;
    std::unique_ptr<SpectatorManager> spectatorManager_;
    
    // 协议层
    std::unique_ptr<MessageDispatcher> messageDispatcher_;
};

} // namespace gomoku

#endif // GAME_SERVER_H