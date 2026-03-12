# 五子棋服务器架构报告

## 1. 系统概述

### 1.1 项目背景
网络联机五子棋对战系统是一个支持多人在线对战的实时游戏服务器，采用C/S架构，提供用户认证、房间管理、实时对战、观战系统、聊天功能等完整功能。

### 1.2 技术栈
- **编程语言**：C++14
- **网络模型**：Reactor模式
- **通信协议**：Protobuf 3.21.12
- **数据存储**：MySQL 5.7+ + Redis 5.0+
- **构建工具**：CMake 3.10+
- **线程库**：pthread
- **客户端**：Qt 5.15+

### 1.3 主要功能
- 用户注册和登录
- 房间创建和加入
- 实时对战
- 随机匹配
- 观战系统
- 聊天系统
- 排行榜
- 心跳保活

## 2. 架构设计

### 2.1 整体架构
```
客户端
  ↓
网络层（Reactor模式）
  ↓
协议层（Protobuf）
  ↓
业务逻辑层
  ├─ UserManager
  ├─ RoomManager
  ├─ GameController
  ├─ MatchManager
  ├─ ChatManager
  └─ SpectatorManager
  ↓
存储层
  ├─ MySQL
  └─ Redis
```

### 2.2 分层架构说明

#### 客户端层
- **职责**：用户界面、游戏渲染、网络通信
- **技术**：Qt 5.15+
- **通信**：TCP + Protobuf

#### 网络层
- **职责**：处理网络I/O、连接管理、事件分发
- **模式**：Reactor模式
- **核心组件**：
  - EventLoop：事件循环
  - Poller：I/O多路复用（epoll）
  - Channel：事件通道
  - TcpConnection：连接管理
  - Acceptor：连接接受器

#### 协议层
- **职责**：消息编解码、消息分发
- **协议**：Protobuf
- **核心组件**：
  - ProtobufCodec：编解码器
  - MessageDispatcher：消息分发器

#### 业务逻辑层
- **职责**：游戏逻辑、业务规则
- **核心组件**：
  - UserManager：用户管理
  - RoomManager：房间管理
  - GameController：游戏控制
  - MatchManager：匹配系统
  - ChatManager：聊天管理
  - SpectatorManager：观战管理

#### 存储层
- **职责**：数据持久化、缓存管理
- **核心组件**：
  - MySQLClient：MySQL客户端
  - RedisClient：Redis客户端

#### 工具层
- **职责**：通用工具
- **核心组件**：
  - Logger：日志系统
  - ThreadPool：线程池

## 3. 核心模块详解

### 3.1 网络层

#### EventLoop（事件循环）
- **文件**：`include/network/EventLoop.h`, `src/network/EventLoop.cpp`
- **职责**：
  - 管理事件循环
  - 驱动Poller等待I/O事件
  - 管理待处理任务队列
  - 支持定时器

- **核心方法**：
  ```cpp
  void loop();              // 启动事件循环
  void quit();              // 退出事件循环
  void runInLoop(Functor cb); // 在事件循环中执行任务
  ```

#### Poller（I/O多路复用）
- **文件**：`include/network/Poller.h`, `src/network/Poller.cpp`
- **职责**：
  - 封装epoll I/O多路复用
  - 监听文件描述符事件
  - 返回活跃Channel列表

- **核心方法**：
  ```cpp
  int poll(int timeoutMs, ChannelList* activeChannels); // 等待事件
  void updateChannel(Channel* channel);                 // 更新通道
  void removeChannel(Channel* channel);                 // 移除通道
  ```

#### Channel（事件通道）
- **文件**：`include/network/Channel.h`, `src/network/Channel.cpp`
- **职责**：
  - 封装文件描述符和回调函数
  - 管理读/写/错误/关闭事件
  - 绑定到EventLoop

- **核心方法**：
  ```cpp
  void setReadCallback(ReadCallback cb);    // 设置读回调
  void setWriteCallback(WriteCallback cb);  // 设置写回调
  void setCloseCallback(CloseCallback cb);  // 设置关闭回调
  void setErrorCallback(ErrorCallback cb);  // 设置错误回调
  void enableReading();                     // 启用读事件
  void enableWriting();                     // 启用写事件
  void disableAll();                        // 禁用所有事件
  ```

#### TcpConnection（TCP连接）
- **文件**：`include/network/TcpConnection.h`, `src/network/TcpConnection.cpp`
- **职责**：
  - 管理单个TCP连接
  - 处理读写缓冲区
  - 优雅关闭支持

- **核心方法**：
  ```cpp
  void send(const std::string& message);  // 发送消息
  void shutdown();                        // 关闭连接
  void forceClose();                      // 强制关闭
  void setConnectionCallback(ConnectionCallback cb);  // 设置连接回调
  void setMessageCallback(MessageCallback cb);        // 设置消息回调
  void setCloseCallback(CloseCallback cb);            // 设置关闭回调
  ```

#### Acceptor（连接接受器）
- **文件**：`include/network/Acceptor.h`, `src/network/Acceptor.cpp`
- **职责**：
  - 监听新连接
  - 创建TcpConnection
  - 接受连接回调

- **核心方法**：
  ```cpp
  void setNewConnectionCallback(NewConnectionCallback cb);  // 设置新连接回调
  void listen();                                            // 开始监听
  ```

### 3.2 协议层

#### ProtobufCodec（编解码器）
- **文件**：`include/protocol/ProtobufCodec.h`, `src/protocol/ProtobufCodec.cpp`
- **职责**：
  - 消息编码：序列化为字节流
  - 消息解码：从字节流反序列化
  - 消息工厂：根据类型创建消息对象
  - 错误处理：解码失败回调

- **消息格式**：
  ```
  [4字节长度][4字节类型][Protobuf消息体]
  ```

- **核心方法**：
  ```cpp
  std::string encode(const google::protobuf::Message& message);  // 编码
  bool decode(const char* buf, int len, google::protobuf::Message** message);  // 解码
  void setErrorCallback(ErrorCallback cb);  // 设置错误回调
  ```

#### MessageDispatcher（消息分发器）
- **文件**：`include/protocol/MessageDispatcher.h`, `src/protocol/MessageDispatcher.cpp`
- **职责**：
  - 注册消息处理回调
  - 根据消息类型分发
  - 类型安全转换

- **核心方法**：
  ```cpp
  template<typename T>
  void registerMessage(MessageCallback<T> cb);  // 注册消息处理器
  void dispatch(int messageType, const std::string& data);  // 分发消息
  ```

### 3.3 业务逻辑层

#### UserManager（用户管理器）
- **文件**：`include/business/UserManager.h`, `src/business/UserManager.cpp`
- **职责**：
  - 用户登录/注册/登出
  - Token生成和验证
  - 在线用户管理
  - 战绩更新
  - 排行榜查询

- **核心方法**：
  ```cpp
  int login(const std::string& username, const std::string& password, std::string& token);  // 登录
  int registerUser(const std::string& username, const std::string& password);  // 注册
  bool logout(int userId);  // 登出
  bool validateToken(int userId, const std::string& token);  // 验证Token
  std::vector<InternalUserInfo> getRankList(int type, int limit);  // 获取排行榜
  ```

#### RoomManager（房间管理器）
- **文件**：`include/business/RoomManager.h`, `src/business/RoomManager.cpp`
- **职责**：
  - 创建/关闭房间
  - 加入/离开房间
  - 房间状态管理
  - 观战者管理
  - 房间广播

- **房间状态机**：
  ```
  WAITING (等待玩家) → GAMING (游戏中) → FINISHED (已结束)
  ```

- **核心方法**：
  ```cpp
  std::string createRoom(int userId, const std::string& roomName);  // 创建房间
  bool joinRoom(int userId, const std::string& roomId);  // 加入房间
  bool leaveRoom(int userId, const std::string& roomId);  // 离开房间
  std::vector<InternalRoomInfo> getRoomList();  // 获取房间列表
  bool closeRoom(const std::string& roomId);  // 关闭房间
  ```

#### GameController（游戏控制器）
- **文件**：`include/business/GameController.h`, `src/business/GameController.cpp`
- **职责**：
  - 棋盘管理（18x18）
  - 落子合法性验证
  - 胜负判定（五子连珠）
  - 悔棋功能
  - 棋步历史记录

- **游戏规则**：
  ```cpp
  static const int BOARD_SIZE = 18;
  static const int WIN_COUNT = 5;
  ```

- **核心方法**：
  ```cpp
  bool initGame(int player1Id, int player2Id);  // 初始化游戏
  bool makeMove(int x, int y, int playerId);    // 落子
  bool undoMove();                              // 悔棋
  GameResult checkWin(int x, int y, int playerId);  // 检查胜负
  void resetGame();                             // 重置游戏
  std::vector<Move> getMoveHistory();           // 获取棋步历史
  ```

#### MatchManager（匹配管理器）
- **文件**：`include/business/MatchManager.h`, `src/business/MatchManager.cpp`
- **职责**：
  - 匹配队列管理（FIFO）
  - 自动配对（队列>=2人时触发）
  - 临时房间创建
  - 匹配成功通知

- **匹配流程**：
  ```
  玩家加入队列 → 队列检测 → 配对成功 → 创建房间 → 通知双方
  ```

- **核心方法**：
  ```cpp
  bool addToMatchQueue(int userId);  // 加入匹配队列
  bool removeFromMatchQueue(int userId);  // 移除匹配队列
  void checkMatch();  // 检查匹配
  int getMatchQueueSize();  // 获取匹配队列大小
  ```

#### ChatManager（聊天管理器）
- **文件**：`include/business/ChatManager.h`, `src/business/ChatManager.cpp`
- **职责**：
  - 大厅聊天（广播所有在线用户）
  - 私聊（点对点通信）
  - 房间聊天（房间内广播）
  - 敏感词过滤
  - 聊天历史存储

- **聊天类型**：
  ```cpp
  enum class InternalChatType { LOBBY, PRIVATE, ROOM };
  ```

- **核心方法**：
  ```cpp
  bool sendLobbyMessage(int senderId, const std::string& message);  // 发送大厅消息
  bool sendPrivateMessage(int senderId, int receiverId, const std::string& message);  // 发送私聊
  bool sendRoomMessage(int senderId, const std::string& roomId, const std::string& message);  // 发送房间消息
  std::vector<InternalChatMessage> getChatHistory(InternalChatType type, const std::string& target, int limit);  // 获取聊天历史
  ```

#### SpectatorManager（观战管理器）
- **文件**：`include/business/SpectatorManager.h`, `src/business/SpectatorManager.cpp`
- **职责**：
  - 加入/离开观战
  - 获取观战者列表
  - 获取历史棋步
  - 实时同步棋局

- **核心方法**：
  ```cpp
  bool joinSpectate(int userId, const std::string& roomId);  // 加入观战
  bool leaveSpectate(int userId, const std::string& roomId);  // 离开观战
  std::vector<int> getSpectators(const std::string& roomId);  // 获取观战者列表
  std::vector<Move> getGameMoves(const std::string& roomId);  // 获取棋步
  ```

### 3.4 存储层

#### MySQLClient（MySQL客户端）
- **文件**：`include/storage/MySQLClient.h`, `src/storage/MySQLClient.cpp`
- **职责**：
  - 连接池管理（默认10个连接）
  - 查询结果封装
  - 事务支持
  - 自动资源管理

- **数据库表结构**：
  ```sql
  users          # 用户信息表
  game_records   # 对局记录表
  chat_history   # 聊天记录表
  ```

- **核心方法**：
  ```cpp
  MySQLResult executeQuery(const std::string& sql);  // 执行查询
  int executeUpdate(const std::string& sql);  // 执行更新
  int getLastInsertId();  // 获取最后插入ID
  void beginTransaction();  // 开始事务
  void commit();  // 提交事务
  void rollback();  // 回滚事务
  ```

#### RedisClient（Redis客户端）
- **文件**：`include/storage/RedisClient.h`, `src/storage/RedisClient.cpp`
- **职责**：
  - 单连接模式
  - 支持多种数据结构
  - 缓存管理

- **Redis数据结构**：
  ```
  # 用户相关
  online:user:{id}          → 用户连接信息
  online_users              → 有序集合 (在线用户)

  # 房间相关
  room:{room_id}            → Hash (房间信息)
  rooms:active              → Set (活跃房间)
  room:{room_id}:members    → Set (房间成员)
  room:{room_id}:spectators → Set (观战者)
  room:{room_id}:moves      → List (棋步历史)

  # 匹配相关
  match_queue               → List (匹配队列)

  # 聊天相关
  chat:lobby                → ZSet (大厅聊天)
  chat:private:{a}:{b}      → ZSet (私聊历史)
  chat:room:{room_id}       → ZSet (房间聊天)
  ```

- **核心方法**：
  ```cpp
  bool set(const std::string& key, const std::string& value);  // 设置键值
  std::string get(const std::string& key);  // 获取键值
  bool hset(const std::string& key, const std::string& field, const std::string& value);  // 设置Hash字段
  std::string hget(const std::string& key, const std::string& field);  // 获取Hash字段
  bool lpush(const std::string& key, const std::string& value);  // 左推入列表
  std::string rpop(const std::string& key);  // 右弹出列表
  bool sadd(const std::string& key, const std::string& member);  // 添加到集合
  bool srem(const std::string& key, const std::string& member);  // 从集合移除
  ```

### 3.5 工具层

#### Logger（日志系统）
- **文件**：`include/utils/Logger.h`, `src/utils/Logger.cpp`
- **职责**：
  - 分级日志（DEBUG/INFO/WARN/ERROR）
  - 日志轮转
  - 异步日志写入

- **核心方法**：
  ```cpp
  void debug(const std::string& message);  // DEBUG级别
  void info(const std::string& message);   // INFO级别
  void warn(const std::string& message);   // WARN级别
  void error(const std::string& message);  // ERROR级别
  ```

#### ThreadPool（线程池）
- **文件**：`include/utils/ThreadPool.h`, `src/utils/ThreadPool.cpp`
- **职责**：
  - 线程池管理
  - 任务队列
  - 任务调度

- **核心方法**：
  ```cpp
  void start(int numThreads);  // 启动线程池
  void submit(Task task);      // 提交任务
  void stop();                 // 停止线程池
  ```

## 4. 数据流

### 4.1 消息处理流程
```
客户端发送消息
  ↓
TcpConnection接收数据
  ↓
ProtobufCodec解码
  ↓
MessageDispatcher分发
  ↓
业务逻辑处理（UserManager/RoomManager/GameController等）
  ↓
存储层查询/更新（MySQL/Redis）
  ↓
生成响应
  ↓
ProtobufCodec编码
  ↓
TcpConnection发送
  ↓
客户端接收响应
```

### 4.2 用户登录流程
```
客户端 → LoginRequest
  ↓
GameServer接收
  ↓
UserManager验证
  ↓
查询MySQL用户数据
  ↓
验证密码
  ↓
生成Token
  ↓
缓存到Redis
  ↓
返回LoginResponse
  ↓
客户端 ← LoginResponse
```

### 4.3 创建房间流程
```
客户端 → CreateRoomRequest
  ↓
GameServer接收
  ↓
RoomManager创建房间
  ↓
生成房间号（6位随机）
  ↓
存储到Redis
  ↓
添加到活跃房间列表
  ↓
返回CreateRoomResponse
  ↓
客户端 ← CreateRoomResponse
  ↓
广播房间列表更新
  ↓
所有在线用户 ← RoomListResponse
```

### 4.4 对战流程
```
玩家1 → StartGameRequest
  ↓
GameServer接收
  ↓
通知房间内所有玩家
  ↓
玩家2 ← GameStartedNotify

循环落子：
  玩家1 → MoveRequest
    ↓
  GameServer → GameController
    ↓
  验证合法性
    ↓
  检查胜负
    ↓
  保存棋步
    ↓
  广播落子
    ↓
  玩家2 ← MoveNotify
    ↓
  检查游戏结束
    ↓
  玩家2 ← GameOverNotify (如结束)
    ↓
  更新战绩到MySQL
```

## 5. 并发模型

### 5.1 Reactor模式
- **主线程**：接受新连接
- **IO线程池**：处理IO事件
- **工作线程池**：处理业务逻辑

### 5.2 线程安全
- 使用互斥锁保护共享数据
- 使用原子操作进行状态同步
- 使用消息队列进行线程间通信

### 5.3 事件循环
```
EventLoop::loop()
  ↓
Poller::poll() (epoll_wait)
  ↓
返回活跃Channel列表
  ↓
Channel::handleEvent() (调用回调)
  ↓
TcpConnection::handleRead/handleWrite
  ↓
业务逻辑处理
```

## 6. 存储设计

### 6.1 MySQL设计
- **users表**：用户信息
  - user_id: 主键
  - username: 唯一索引
  - password_hash: bcrypt加密
  - win_count, lose_count, draw_count: 战绩
  - rating: 天梯积分

- **game_records表**：对局记录
  - game_id: 主键
  - player1_id, player2_id: 外键
  - winner_id: 获胜者
  - game_mode: AI/ONLINE/MATCH
  - moves_data: JSON格式棋步

- **chat_history表**：聊天记录
  - message_id: 主键
  - sender_id: 外键
  - receiver_id: 私聊目标
  - room_id: 房间ID
  - message_type: LOBBY/PRIVATE/ROOM

### 6.2 Redis设计
- **在线用户缓存**：user:token:{token} → userId
- **房间信息缓存**：room:{room_id} → Hash
- **匹配队列**：match_queue → List
- **聊天历史**：chat:{type}:{target} → ZSet

## 7. 性能优化

### 7.1 网络优化
- 使用非阻塞IO
- 零拷贝技术
- 连接池复用

### 7.2 存储优化
- Redis缓存热点数据
- MySQL连接池
- 批量操作优化

### 7.3 并发优化
- 线程池复用
- 事件驱动模型
- 异步IO

## 8. 安全设计

### 8.1 认证机制
- 用户名密码认证
- Token认证
- 心跳保活

### 8.2 数据安全
- 密码加密存储
- SQL注入防护
- XSS防护

## 9. 监控和日志

### 9.1 日志系统
- 分级日志（DEBUG/INFO/WARN/ERROR）
- 日志轮转
- 异步日志写入

### 9.2 监控指标
- 在线用户数
- 房间数
- 消息吞吐量
- 响应时间

## 10. 部署架构

### 10.1 单机部署
```
[客户端] ←→ [服务器]
                ↓
           [MySQL] + [Redis]
```

### 10.2 分布式部署
```
[客户端] ←→ [负载均衡] ←→ [服务器集群]
                                 ↓
                          [MySQL集群] + [Redis集群]
```

## 11. 扩展性设计

### 11.1 水平扩展
- 无状态服务器设计
- 一致性哈希负载均衡
- 数据分片

### 11.2 功能扩展
- 插件化架构
- 消息注册机制
- 策略模式

---

**报告生成时间**：2026-03-12  
**报告生成者**：iFlow CLI  
**版本**：1.0.0