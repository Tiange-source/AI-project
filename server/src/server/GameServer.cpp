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
    mysqlClient_.reset(new MySQLClient());
    MySQLConfig mysqlConfig;
    mysqlConfig.host = mysqlHost_;
    mysqlConfig.port = mysqlPort_;
    mysqlConfig.username = mysqlUser_;
    mysqlConfig.password = mysqlPassword_;
    mysqlConfig.database = mysqlDatabase_;
    if (!mysqlClient_->initialize(mysqlConfig)) {
        LOG_ERROR("GameServer::initialize - failed to connect to MySQL");
        return false;
    }
    
    // 初始化Redis客户端
    redisClient_.reset(new RedisClient());
    RedisConfig redisConfig;
    redisConfig.host = redisHost_;
    redisConfig.port = redisPort_;
    if (!redisClient_->initialize(redisConfig)) {
        LOG_ERROR("GameServer::initialize - failed to connect to Redis");
        return false;
    }
    
    // 初始化业务层
    userManager_.reset(new UserManager());
    if (!userManager_->initialize(mysqlClient_.get(), redisClient_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize UserManager");
        return false;
    }
    
    roomManager_.reset(new RoomManager());
    if (!roomManager_->initialize(redisClient_.get(), userManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize RoomManager");
        return false;
    }
    
    gameController_.reset(new GameController());
    
    matchManager_.reset(new MatchManager());
    if (!matchManager_->initialize(redisClient_.get(), userManager_.get(), roomManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize MatchManager");
        return false;
    }
    
    chatManager_.reset(new ChatManager());
    if (!chatManager_->initialize(redisClient_.get(), userManager_.get(), roomManager_.get())) {
        LOG_ERROR("GameServer::initialize - failed to initialize ChatManager");
        return false;
    }
    
    spectatorManager_.reset(new SpectatorManager());
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
    messageDispatcher_.reset(new MessageDispatcher());
    
    // 注册消息处理器
    // 这里简化处理，实际应该为每个消息类型注册处理器
    
    // 创建Acceptor
    acceptor_.reset(new Acceptor(&eventLoop_, "0.0.0.0", port_));
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
    
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(&eventLoop_, std::string("conn_") + std::to_string(connId), sockfd, peerIp, peerPort);
    
    // 设置回调
    conn->setCloseCallback([this](const TcpConnectionPtr& conn) { this->onConnectionClosed(conn); });
    conn->setMessageCallback([this](const TcpConnectionPtr& conn, Buffer* buffer) { this->onMessage(conn, buffer); });
    
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
    // 注意：ProtobufCodec::decode会直接调用messageCallback，所以我们不需要手动提取消息
    // 我们需要设置ProtobufCodec的回调函数
    static ProtobufCodec codec;
    
    static bool codecInitialized = false;
    if (!codecInitialized) {
        codecInitialized = true;
        
        // 设置消息回调
        codec.setMessageCallback([this](const TcpConnectionPtr& conn, const ProtobufMessagePtr& message) {
            // 获取userId
            int userId = 0;
            auto it = connIdToUserId_.find(conn->getConnId());
            if (it != connIdToUserId_.end()) {
                userId = it->second;
            }
            
            // 根据消息类型分发到对应处理器
            this->routeMessage(userId, message);
        });
        
        // 设置错误回调
        codec.setErrorCallback([this](const TcpConnectionPtr& conn, Buffer* buf, int errorCode, const std::string& errorMessage) {
            LOG_ERROR("Protocol codec error: " + errorMessage + " (" + std::to_string(errorCode) + ")");
            // 可以选择关闭连接
        });
    }
    
    // 解码消息
    codec.decode(conn, buffer);
}

void GameServer::routeMessage(int userId, const ProtobufMessagePtr& message) {
    if (!message) {
        LOG_ERROR("GameServer::routeMessage - null message");
        return;
    }
    
    const std::string& typeName = message->GetTypeName();
    
    // 根据消息类型分发到对应处理器
    if (typeName == "gomoku.LoginRequest") {
        auto* request = static_cast<gomoku::LoginRequest*>(message.get());
        handleLogin(userId, request->username(), request->password());
    }
    else if (typeName == "gomoku.RegisterRequest") {
        auto* request = static_cast<gomoku::RegisterRequest*>(message.get());
        handleRegister(userId, request->username(), request->password(), request->email(), request->nickname());
    }
    else if (typeName == "gomoku.CreateRoomRequest") {
        auto* request = static_cast<gomoku::CreateRoomRequest*>(message.get());
        handleCreateRoom(userId, request->room_name(), request->password());
    }
    else if (typeName == "gomoku.JoinRoomRequest") {
        auto* request = static_cast<gomoku::JoinRoomRequest*>(message.get());
        handleJoinRoom(userId, request->room_id(), request->password());
    }
    else if (typeName == "gomoku.LeaveRoomRequest") {
        handleLeaveRoom(userId);
    }
    else if (typeName == "gomoku.StartGameRequest") {
        handleStartGame(userId);
    }
    else if (typeName == "gomoku.MoveRequest") {
        auto* request = static_cast<gomoku::MoveRequest*>(message.get());
        handleMove(userId, request->row(), request->col());
    }
    else if (typeName == "gomoku.ChatMessageRequest") {
        auto* request = static_cast<gomoku::ChatMessageRequest*>(message.get());
        ChatType type = static_cast<ChatType>(request->type());
        handleChatMessage(userId, type, request->target_id(), request->room_id(), request->content());
    }
    else if (typeName == "gomoku.SpectateRequest") {
        auto* request = static_cast<gomoku::SpectateRequest*>(message.get());
        handleSpectate(userId, request->room_id());
    }
    else if (typeName == "gomoku.RandomMatchRequest") {
        handleRandomMatch(userId);
    }
    else if (typeName == "gomoku.RankListRequest") {
        auto* request = static_cast<gomoku::RankListRequest*>(message.get());
        RankType type = static_cast<RankType>(request->type());
        handleRankList(userId, type, request->offset(), request->limit());
    }
    else {
        LOG_WARN("Unknown message type: " + typeName);
    }
}

void GameServer::handleLogin(int connId, const std::string& username, const std::string& password) {
    // 调用UserManager进行登录验证
    InternalUserInfo userInfo;
    std::string token;
    bool success = userManager_->login(username, password, userInfo, token);
    
    // 创建Protobuf响应
    gomoku::LoginResponse response;
    response.set_success(success);
    
    if (success) {
        response.set_token(token);
        
        // 设置用户信息
        auto* user_info = response.mutable_user_info();
        user_info->set_user_id(userInfo.userId);
        user_info->set_username(userInfo.username);
        user_info->set_nickname(userInfo.nickname);
        user_info->set_avatar_url(userInfo.avatarUrl);
        user_info->set_win_count(userInfo.winCount);
        user_info->set_lose_count(userInfo.loseCount);
        user_info->set_draw_count(userInfo.drawCount);
        user_info->set_rating(userInfo.rating);
        user_info->set_total_games(userInfo.totalGames);
        
        // 保存userId和connId的映射
        connIdToUserId_[connId] = userInfo.userId;
        userIdToConnId_[userInfo.userId] = connId;
        
        LOG_INFO("User login success: " + username);
    } else {
        response.set_error_msg("Invalid username or password");
        LOG_INFO("User login failed: " + username);
    }
    
    // 使用ProtobufCodec编码并发送响应
    ProtobufCodec codec;
    std::string encoded = codec.encode(response);
    
    // 发送给对应的连接
    auto it = connections_.find(connId);
    if (it != connections_.end()) {
        it->second->send(encoded);
    }
}

void GameServer::handleRegister(int connId, const std::string& username, const std::string& password,
                                 const std::string& email, const std::string& nickname) {
    // 调用UserManager进行注册
    InternalUserInfo userInfo;
    bool success = userManager_->registerUser(username, password, email, nickname, userInfo);
    
    // 创建Protobuf响应
    gomoku::RegisterResponse response;
    response.set_success(success);
    
    if (success) {
        // 设置用户信息
        auto* user_info = response.mutable_user_info();
        user_info->set_user_id(userInfo.userId);
        user_info->set_username(userInfo.username);
        user_info->set_nickname(userInfo.nickname);
        user_info->set_avatar_url(userInfo.avatarUrl);
        user_info->set_win_count(userInfo.winCount);
        user_info->set_lose_count(userInfo.loseCount);
        user_info->set_draw_count(userInfo.drawCount);
        user_info->set_rating(userInfo.rating);
        user_info->set_total_games(userInfo.totalGames);
        
        LOG_INFO("User register success: " + username);
    } else {
        response.set_error_msg("Username already exists");
        LOG_INFO("User register failed: " + username);
    }
    
    // 使用ProtobufCodec编码并发送响应
    ProtobufCodec codec;
    std::string encoded = codec.encode(response);
    
    // 发送给对应的连接
    auto it = connections_.find(connId);
    if (it != connections_.end()) {
        it->second->send(encoded);
    }
}

void GameServer::handleCreateRoom(int userId, const std::string& roomName, const std::string& password) {
    InternalRoomInfo roomInfo;
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
    InternalRoomInfo roomInfo;
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
    InternalRoomInfo roomInfo;
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
    if (gameController_->checkWin(row, col, userId) == GameResult::WIN) {
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
        switch (type) {
            case 0: // 胜场数
                response << entry.userId << ":" << entry.nickname << ":" << entry.winCount << ";";
                break;
            case 1: // 积分
                response << entry.userId << ":" << entry.nickname << ":" << entry.rating << ";";
                break;
            case 2: // 胜率
                response << entry.userId << ":" << entry.nickname << ":" << (entry.totalGames > 0 ? (entry.winCount * 100.0 / entry.totalGames) : 0) << ";";
                break;
            default:
                response << entry.userId << ":" << entry.nickname << ":" << entry.rating << ";";
                break;
        }
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
            std::string encoded = message;
            connIt->second->send(encoded);
        }
    }
}

void GameServer::broadcastToRoom(const std::string& roomId, const std::string& message) {
    InternalRoomInfo roomInfo;
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