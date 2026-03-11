#include "server/GameServer.h"
#include "utils/Logger.h"
#include "utils/ThreadPool.h"
#include "protocol/ProtobufCodec.h"
#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <sstream>

namespace gomoku {

GameServer::GameServer() 
    : port_(8888), mysqlPort_(3306), redisPort_(6379) {
}

GameServer::~GameServer() {
    stop();
}

bool GameServer::loadConfig(const std::string& configFile) {
    configFile_ = configFile;
    
    std::ifstream file(configFile);
    if (!file.is_open()) {
        LOG_ERROR("GameServer::loadConfig - failed to open config file: " + configFile);
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除前后空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            if (key == "port") {
                port_ = std::stoi(value);
            } else if (key == "mysql_host") {
                mysqlHost_ = value;
            } else if (key == "mysql_port") {
                mysqlPort_ = std::stoi(value);
            } else if (key == "mysql_user") {
                mysqlUser_ = value;
            } else if (key == "mysql_password") {
                mysqlPassword_ = value;
            } else if (key == "mysql_database") {
                mysqlDatabase_ = value;
            } else if (key == "redis_host") {
                redisHost_ = value;
            } else if (key == "redis_port") {
                redisPort_ = std::stoi(value);
            }
        }
    }
    
    file.close();
    LOG_INFO("GameServer::loadConfig - config loaded from " + configFile);
    return true;
}

bool GameServer::initialize() {
    // 初始化MySQL客户端
    mysqlClient_ = std::make_unique<MySQLClient>();
    if (!mysqlClient_->connect(mysqlHost_, mysqlPort_, mysqlUser_, mysqlPassword_, mysqlDatabase_)) {
        LOG_ERROR("GameServer::initialize - failed to connect to MySQL");
        return false;
    }
    
    // 初始化Redis客户端
    redisClient_ = std::make_unique<RedisClient>();
    if (!redisClient_->connect(redisHost_, redisPort_)) {
        LOG_ERROR("GameServer::initialize - failed to connect to Redis");
        return false;
    }
    
    // 初始化业务层
    userManager_ = std::make_unique<UserManager>();
    if (!userManager_->initialize(mysqlClient_.get(), redisClient_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize UserManager");
        return false;
    }
    
    roomManager_ = std::make_unique<RoomManager>();
    if (!roomManager_->initialize(redisClient_.get(), userManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize RoomManager");
        return false;
    }
    
    gameController_ = std::make_unique<GameController>();
    
    matchManager_ = std::make_unique<MatchManager>();
    if (!matchManager_->initialize(redisClient_.get(), userManager_.get(), roomManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize MatchManager");
        return false;
    }
    
    chatManager_ = std::make_unique<ChatManager>();
    if (!chatManager_->initialize(redisClient_.get(), userManager_.get(), roomManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize ChatManager");
        return false;
    }
    
    spectatorManager_ = std::make_unique<SpectatorManager>();
    if (!spectatorManager_->initialize(redisClient_.get(), userManager_.get(), roomManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize SpectatorManager");
        return false;
    }
    
    // 设置广播回调
    auto broadcastCb = [this](const std::vector<int>& userIds, const std::string& message) {
        for (int userId : userIds) {
            broadcast(userId, message);
        }
    };
    
    chatManager_->setBroadcastCallback(broadcastCb);
    
    // 初始化消息分发器
    messageDispatcher_ = std::make_unique<MessageDispatcher>();
    
    // 注册消息处理器
    // 这里简化处理，实际应该为每个消息类型注册处理器
    
    // 创建Acceptor
    acceptor_ = std::make_unique<Acceptor>(&eventLoop_, port_);
    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const std::string& peerIp, uint16_t peerPort) {
            this->onNewConnection(sockfd, peerIp, peerPort);
        }
    );
    
    LOG_INFO("GameServer initialized");
    return true;
}

bool GameServer::start() {
    LOG_INFO("GameServer starting on port " + std::to_string(port_));
    
    // 开始监听
    acceptor_->listen();
    
    // 启动事件循环
    eventLoop_.loop();
    
    LOG_INFO("GameServer stopped");
    return true;
}

void GameServer::stop() {
    LOG_INFO("GameServer stopping...");
    eventLoop_.quit();
}

void GameServer::onNewConnection(int sockfd, const std::string& peerIp, uint16_t peerPort) {
    int connId = sockfd; // 简化：使用sockfd作为connId
    
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(&eventLoop_, connId, sockfd);
    
    // 设置回调
    conn->setCloseCallback([this, connId]() { this->onConnectionClosed(conn); });
    conn->setMessageCallback([this, conn](Buffer* buffer) { this->onMessage(conn, buffer); });
    
    // 建立连接
    conn->connectEstablished();
    
    connections_[connId] = conn;
    
    LOG_INFO("New connection from " + peerIp + ":" + std::to_string(peerPort));
}

void GameServer::onConnectionClosed(const TcpConnectionPtr& conn) {
    int connId = conn->getConnId();
    
    // 获取userId
    int userId = 0;
    auto it = connIdToUserId_.find(connId);
    if (it != connIdToUserId_.end()) {
        userId = it->second;
        connIdToUserId_.erase(it);
        userIdToConnId_.erase(userId);
        
        // 用户登出
        userManager_->logout(userId);
        
        // 如果用户在房间中，离开房间
        roomManager_->leaveRoom(userId);
    }
    
    // 移除连接
    connections_.erase(connId);
    
    LOG_INFO("Connection closed, connId=" + std::to_string(connId) + ", userId=" + std::to_string(userId));
}

void GameServer::onMessage(const TcpConnectionPtr& conn, Buffer* buffer) {
    // 使用ProtobufCodec解析消息
    std::vector<std::pair<MessageType, std::string>> messages;
    ProtobufCodec::decode(buffer, messages);
    
    for (const auto& msg : messages) {
        MessageType type = msg.first;
        const std::string& data = msg.second;
        
        // 获取userId
        int userId = 0;
        auto it = connIdToUserId_.find(conn->getConnId());
        if (it != connIdToUserId_.end()) {
            userId = it->second;
        }
        
        // 路由消息
        routeMessage(userId, type, data);
    }
}

void GameServer::routeMessage(int userId, MessageType type, const std::string& message) {
    // 根据消息类型分发到对应处理器
    // 这里简化处理，实际应该解析Protobuf消息
    
    switch (type) {
        case MessageType::LOGIN_REQUEST:
            // 解析登录请求
            handleLogin(userId, "username", "password");
            break;
            
        case MessageType::REGISTER_REQUEST:
            // 解析注册请求
            handleRegister(userId, "username", "password", "email", "nickname");
            break;
            
        case MessageType::CREATE_ROOM_REQUEST:
            // 解析创建房间请求
            handleCreateRoom(userId, "Room Name", "");
            break;
            
        case MessageType::JOIN_ROOM_REQUEST:
            // 解析加入房间请求
            handleJoinRoom(userId, "123456", "");
            break;
            
        case MessageType::LEAVE_ROOM_REQUEST:
            handleLeaveRoom(userId);
            break;
            
        case MessageType::START_GAME_REQUEST:
            handleStartGame(userId);
            break;
            
        case MessageType::MOVE_REQUEST:
            // 解析落子请求
            handleMove(userId, 5, 5);
            break;
            
        case MessageType::CHAT_MESSAGE_REQUEST:
            // 解析聊天消息
            handleChatMessage(userId, ChatType::LOBBY, 0, "", "Hello");
            break;
            
        case MessageType::SPECTATE_REQUEST:
            // 解析观战请求
            handleSpectate(userId, "123456");
            break;
            
        case MessageType::RANDOM_MATCH_REQUEST:
            handleRandomMatch(userId);
            break;
            
        case MessageType::RANK_LIST_REQUEST:
            handleRankList(userId, RankType::RATING, 0, 10);
            break;
            
        default:
            LOG_WARN("Unknown message type: " + std::to_string(static_cast<int>(type)));
            break;
    }
}

void GameServer::handleLogin(int connId, const std::string& username, const std::string& password) {
    // 调用UserManager进行登录验证
    UserInfo userInfo;
    std::string token;
    bool success = userManager_->login(username, password, userInfo, token);
    
    // 构造响应
    std::ostringstream response;
    if (success) {
        response << "LOGIN_RESPONSE:success:" << userInfo.userId << ":" 
                << userInfo.nickname << ":" << token;
        
        // 保存userId和connId的映射
        connIdToUserId_[connId] = userInfo.userId;
        userIdToConnId_[userInfo.userId] = connId;
        
        LOG_INFO("User login success: " + username);
    } else {
        response << "LOGIN_RESPONSE:fail:Invalid username or password";
        LOG_INFO("User login failed: " + username);
    }
    
    // 发送响应
    broadcast(userInfo.userId, response.str());
}

void GameServer::handleRegister(int connId, const std::string& username, const std::string& password,
                                 const std::string& email, const std::string& nickname) {
    // 调用UserManager进行注册
    UserInfo userInfo;
    bool success = userManager_->registerUser(username, password, email, nickname, userInfo);
    
    // 构造响应
    std::ostringstream response;
    if (success) {
        response << "REGISTER_RESPONSE:success:" << userInfo.userId << ":" << userInfo.nickname;
        LOG_INFO("User register success: " + username);
    } else {
        response << "REGISTER_RESPONSE:fail:Username already exists";
        LOG_INFO("User register failed: " + username);
    }
    
    // 发送响应（发送给所有连接，简化处理）
    broadcastToAll(response.str());
}

void GameServer::handleCreateRoom(int userId, const std::string& roomName, const std::string& password) {
    RoomInfo roomInfo;
    bool success = roomManager_->createRoom(userId, roomName, password, roomInfo);
    
    std::ostringstream response;
    if (success) {
        response << "CREATE_ROOM_RESPONSE:success:" << roomInfo.roomId;
        LOG_INFO("Room created: " + roomInfo.roomId);
    } else {
        response << "CREATE_ROOM_RESPONSE:fail:Failed to create room";
    }
    
    broadcast(userId, response.str());
}

void GameServer::handleJoinRoom(int userId, const std::string& roomId, const std::string& password) {
    RoomInfo roomInfo;
    bool success = roomManager_->joinRoom(userId, roomId, password, roomInfo);
    
    std::ostringstream response;
    if (success) {
        response << "JOIN_ROOM_RESPONSE:success:" << roomId;
        
        // 通知房间内其他玩家
        std::ostringstream notify;
        notify << "PLAYER_JOINED:" << userId;
        broadcastToRoom(roomId, notify.str());
        
        LOG_INFO("User " + std::to_string(userId) + " joined room " + roomId);
    } else {
        response << "JOIN_ROOM_RESPONSE:fail:Failed to join room";
    }
    
    broadcast(userId, response.str());
}

void GameServer::handleLeaveRoom(int userId) {
    roomManager_->leaveRoom(userId);
    
    std::ostringstream response;
    response << "LEAVE_ROOM_RESPONSE:success";
    
    broadcast(userId, response.str());
    
    LOG_INFO("User " + std::to_string(userId) + " left room");
}

void GameServer::handleStartGame(int userId) {
    // 获取用户所在的房间
    RoomInfo roomInfo;
    // 简化处理，实际需要从Redis查询用户所在的房间
    
    std::ostringstream response;
    response << "START_GAME_RESPONSE:success";
    
    broadcast(userId, response.str());
    
    LOG_INFO("Game started by user " + std::to_string(userId));
}

void GameServer::handleMove(int userId, int row, int col) {
    // 验证落子
    if (!gameController_->isValidMove(row, col)) {
        std::ostringstream response;
        response << "MOVE_RESPONSE:fail:Invalid move";
        broadcast(userId, response.str());
        return;
    }
    
    // 执行落子
    gameController_->makeMove(row, col, userId);
    
    // 检查胜负
    if (gameController_->checkWin(row, col, userId)) {
        std::ostringstream gameOver;
        gameOver << "GAME_OVER:" << userId << ":FIVE_IN_ROW";
        broadcastToAll(gameOver.str());
        
        LOG_INFO("Game over, winner: " + std::to_string(userId));
    }
    
    std::ostringstream response;
    response << "MOVE_RESPONSE:success";
    broadcast(userId, response.str());
    
    // 通知对手
    std::ostringstream notify;
    notify << "MOVE_NOTIFY:" << userId << ":" << row << ":" << col;
    broadcastToAll(notify.str());
}

void GameServer::handleChatMessage(int userId, ChatType type, int targetId, const std::string& roomId, const std::string& content) {
    bool success = false;
    
    switch (type) {
        case ChatType::LOBBY:
            success = chatManager_->sendLobbyChat(userId, content);
            break;
            
        case ChatType::PRIVATE:
            success = chatManager_->sendPrivateChat(userId, targetId, content);
            break;
            
        case ChatType::ROOM:
            success = chatManager_->sendRoomChat(userId, roomId, content);
            break;
    }
    
    if (!success) {
        LOG_WARN("Failed to send chat message from user " + std::to_string(userId));
    }
}

void GameServer::handleSpectate(int userId, const std::string& roomId) {
    bool success = spectatorManager_->addSpectator(roomId, userId);
    
    std::ostringstream response;
    if (success) {
        auto history = spectatorManager_->getMoveHistory(roomId);
        
        response << "SPECTATE_RESPONSE:success:" << roomId << ":";
        for (const auto& move : history) {
            response << move << ";";
        }
        
        LOG_INFO("User " + std::to_string(userId) + " started spectating room " + roomId);
    } else {
        response << "SPECTATE_RESPONSE:fail:Failed to spectate room";
    }
    
    broadcast(userId, response.str());
}

void GameServer::handleRandomMatch(int userId) {
    bool success = matchManager_->requestMatch(userId);
    
    std::ostringstream response;
    if (success) {
        int position = matchManager_->getQueuePosition(userId);
        response << "MATCH_WAITING_RESPONSE:success:" << position;
        LOG_INFO("User " + std::to_string(userId) + " requested match, position: " + std::to_string(position));
    } else {
        response << "MATCH_WAITING_RESPONSE:fail:Failed to request match";
    }
    
    broadcast(userId, response.str());
}

void GameServer::handleRankList(int userId, RankType type, int offset, int limit) {
    auto entries = userManager_->getRankList(type, limit);
    
    std::ostringstream response;
    response << "RANK_LIST_RESPONSE:success:";
    
    for (const auto& entry : entries) {
        response << entry.userId << ":" << entry.nickname << ":" << entry.value << ";";
    }
    
    broadcast(userId, response.str());
}

void GameServer::broadcast(int userId, const std::string& message) {
    auto it = userIdToConnId_.find(userId);
    if (it != userIdToConnId_.end()) {
        int connId = it->second;
        auto connIt = connections_.find(connId);
        if (connIt != connections_.end()) {
            // 使用ProtobufCodec编码消息
            std::string encoded = ProtobufCodec::encode(MessageType::LOGIN_RESPONSE, message);
            connIt->second->send(encoded);
        }
    }
}

void GameServer::broadcastToRoom(const std::string& roomId, const std::string& message) {
    RoomInfo roomInfo;
    if (!roomManager_->getRoomInfo(roomId, roomInfo)) {
        return;
    }
    
    // 广播给房间内的所有玩家
    if (roomInfo.player1 != 0) {
        broadcast(roomInfo.player1, message);
    }
    if (roomInfo.player2 != 0) {
        broadcast(roomInfo.player2, message);
    }
    
    // 广播给观战者
    auto spectators = spectatorManager_->getSpectators(roomId);
    for (int spectator : spectators) {
        broadcast(spectator, message);
    }
}

void GameServer::broadcastToAll(const std::string& message) {
    for (const auto& pair : connections_) {
        pair.second->send(message);
    }
}

} // namespace gomoku