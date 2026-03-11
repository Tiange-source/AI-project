# 服务端API文档

## 概述

本文档描述了网络联机五子棋服务端的所有API接口，包括消息类型、请求格式、响应格式和错误码。

## 消息格式

所有消息使用Protobuf序列化，格式如下：

```
[4字节长度][4字节类型][Protobuf消息体]
```

- **长度**：消息体的长度（网络字节序）
- **类型**：消息类型枚举值（网络字节序）
- **消息体**：Protobuf序列化后的数据

## 消息类型定义

### 认证相关 (1-5)

#### LOGIN_REQUEST (1)
登录请求

**请求消息**：`LoginRequest`
```protobuf
message LoginRequest {
    string username = 1;
    string password = 2;
}
```

**响应消息**：`LoginResponse`
```protobuf
message LoginResponse {
    bool success = 1;
    string error_msg = 2;
    string token = 3;
    UserInfo user_info = 4;
}
```

**错误码**：
- `INVALID_CREDENTIALS`: 用户名或密码错误
- `USER_NOT_FOUND`: 用户不存在
- `ALREADY_LOGGED_IN`: 用户已登录

#### REGISTER_REQUEST (3)
注册请求

**请求消息**：`RegisterRequest`
```protobuf
message RegisterRequest {
    string username = 1;
    string password = 2;
    string email = 3;
    string nickname = 4;
}
```

**响应消息**：`RegisterResponse`
```protobuf
message RegisterResponse {
    bool success = 1;
    string error_msg = 2;
    UserInfo user_info = 3;
}
```

**错误码**：
- `USERNAME_EXISTS`: 用户名已存在
- `INVALID_INPUT`: 输入参数无效
- `EMAIL_EXISTS`: 邮箱已被注册

#### LOGOUT_REQUEST (5)
登出请求

**请求消息**：`LogoutRequest`
```protobuf
message LogoutRequest {
    string token = 1;
}
```

**响应消息**：`LogoutResponse`
```protobuf
message LogoutResponse {
    bool success = 1;
    string error_msg = 2;
}
```

### 房间相关 (100-109)

#### CREATE_ROOM_REQUEST (100)
创建房间请求

**请求消息**：`CreateRoomRequest`
```protobuf
message CreateRoomRequest {
    string room_name = 1;
    string password = 2;
}
```

**响应消息**：`CreateRoomResponse`
```protobuf
message CreateRoomResponse {
    bool success = 1;
    string error_msg = 2;
    RoomInfo room_info = 3;
}
```

**错误码**：
- `ROOM_NAME_EXISTS`: 房间名已存在
- `USER_IN_ROOM`: 用户已在房间中
- `MAX_ROOMS_REACHED`: 达到最大房间数限制

#### JOIN_ROOM_REQUEST (102)
加入房间请求

**请求消息**：`JoinRoomRequest`
```protobuf
message JoinRoomRequest {
    string room_id = 1;
    string password = 2;
}
```

**响应消息**：`JoinRoomResponse`
```protobuf
message JoinRoomResponse {
    bool success = 1;
    string error_msg = 2;
    RoomInfo room_info = 3;
}
```

**错误码**：
- `ROOM_NOT_FOUND`: 房间不存在
- `ROOM_FULL`: 房间已满
- `INVALID_PASSWORD`: 密码错误
- `USER_IN_ROOM`: 用户已在房间中

#### LEAVE_ROOM_REQUEST (104)
离开房间请求

**请求消息**：`LeaveRoomRequest`
```protobuf
message LeaveRoomRequest {
    string room_id = 1;
}
```

**响应消息**：`LeaveRoomResponse`
```protobuf
message LeaveRoomResponse {
    bool success = 1;
    string error_msg = 2;
}
```

#### ROOM_LIST_REQUEST (106)
获取房间列表请求

**请求消息**：`RoomListRequest`
```protobuf
message RoomListRequest {
    int32 offset = 1;
    int32 limit = 2;
}
```

**响应消息**：`RoomListResponse`
```protobuf
message RoomListResponse {
    repeated RoomInfo rooms = 1;
    int32 total_count = 2;
}
```

### 游戏相关 (200-208)

#### START_GAME_REQUEST (200)
开始游戏请求

**请求消息**：`StartGameRequest`
```protobuf
message StartGameRequest {
    string room_id = 1;
}
```

**响应消息**：`StartGameResponse`
```protobuf
message StartGameResponse {
    bool success = 1;
    string error_msg = 2;
    int32 first_player_id = 3;
}
```

**错误码**：
- `NOT_ROOM_OWNER`: 不是房主
- `ROOM_NOT_FULL`: 房间未满
- `GAME_ALREADY_STARTED`: 游戏已开始

#### MOVE_REQUEST (203)
落子请求

**请求消息**：`MoveRequest`
```protobuf
message MoveRequest {
    int32 row = 1;
    int32 col = 2;
}
```

**响应消息**：`MoveResponse`
```protobuf
message MoveResponse {
    bool success = 1;
    string error_msg = 2;
}
```

**错误码**：
- `INVALID_POSITION`: 无效位置
- `POSITION_OCCUPIED`: 位置已被占用
- `NOT_YOUR_TURN`: 不是你的回合
- `GAME_NOT_STARTED`: 游戏未开始

#### MOVE_NOTIFY (205)
落子通知（服务器主动推送）

**消息**：`MoveNotify`
```protobuf
message MoveNotify {
    int32 player_id = 1;
    int32 row = 2;
    int32 col = 3;
    int32 next_turn_player_id = 4;
}
```

#### GAME_OVER_NOTIFY (206)
游戏结束通知（服务器主动推送）

**消息**：`GameOverNotify`
```protobuf
message GameOverNotify {
    int32 winner_id = 1;
    GameOverReason reason = 2;
    int32 total_moves = 3;
}
```

**游戏结束原因**：
- `FIVE_IN_ROW`: 五子连珠
- `OPPONENT_QUIT`: 对手退出
- `TIMEOUT`: 超时

### 匹配相关 (300-304)

#### RANDOM_MATCH_REQUEST (300)
随机匹配请求

**请求消息**：`RandomMatchRequest`
```protobuf
message RandomMatchRequest {
}
```

**响应消息**：`MatchWaitingResponse`
```protobuf
message MatchWaitingResponse {
    bool success = 1;
    int32 queue_position = 2;
    int32 estimated_wait_time = 3;
}
```

#### MATCH_SUCCESS_NOTIFY (302)
匹配成功通知（服务器主动推送）

**消息**：`MatchSuccessNotify`
```protobuf
message MatchSuccessNotify {
    string room_id = 1;
    UserInfo opponent_info = 2;
}
```

### 聊天相关 (400-401)

#### CHAT_MESSAGE_REQUEST (400)
聊天消息请求

**请求消息**：`ChatMessageRequest`
```protobuf
message ChatMessageRequest {
    ChatType type = 1;
    int32 target_id = 2;
    string room_id = 3;
    string content = 4;
}
```

**响应消息**：`ChatMessageResponse`
```protobuf
message ChatMessageResponse {
    bool success = 1;
    string error_msg = 2;
}
```

**聊天类型**：
- `LOBBY`: 大厅聊天
- `PRIVATE`: 私聊
- `ROOM`: 房间聊天

#### CHAT_MESSAGE_NOTIFY (401)
聊天消息通知（服务器主动推送）

**消息**：`ChatMessageNotify`
```protobuf
message ChatMessageNotify {
    ChatType type = 1;
    int32 sender_id = 2;
    string sender_name = 3;
    string content = 4;
    int64 timestamp = 5;
}
```

### 观战相关 (500-503)

#### SPECTATE_REQUEST (500)
观战请求

**请求消息**：`SpectateRequest`
```protobuf
message SpectateRequest {
    string room_id = 1;
}
```

**响应消息**：`SpectateResponse`
```protobuf
message SpectateResponse {
    bool success = 1;
    string error_msg = 2;
    RoomInfo room_info = 3;
    repeated Move history_moves = 4;
}
```

**错误码**：
- `ROOM_NOT_FOUND`: 房间不存在
- `GAME_NOT_STARTED`: 游戏未开始
- `GAME_FINISHED`: 游戏已结束

#### SPECTATOR_JOIN_NOTIFY (502)
观战者加入通知（服务器主动推送）

**消息**：`SpectatorJoinNotify`
```protobuf
message SpectatorJoinNotify {
    UserInfo user_info = 1;
    string room_id = 2;
}
```

### 排行榜相关 (600-601)

#### RANK_LIST_REQUEST (600)
获取排行榜请求

**请求消息**：`RankListRequest`
```protobuf
message RankListRequest {
    RankType type = 1;
    int32 offset = 2;
    int32 limit = 3;
}
```

**响应消息**：`RankListResponse`
```protobuf
message RankListResponse {
    repeated RankEntry entries = 1;
    int32 total_count = 2;
}
```

**排行榜类型**：
- `WIN_COUNT`: 胜场排行
- `RATING`: 积分排行
- `WIN_RATE`: 胜率排行

### 心跳相关 (700-701)

#### HEARTBEAT_REQUEST (700)
心跳请求

**请求消息**：`HeartbeatRequest`
```protobuf
message HeartbeatRequest {
    int64 timestamp = 1;
}
```

**响应消息**：`HeartbeatResponse`
```protobuf
message HeartbeatResponse {
    int64 timestamp = 1;
}
```

### 错误通知 (702)

#### ERROR_NOTIFY (702)
错误通知（服务器主动推送）

**消息**：`ErrorNotify`
```protobuf
message ErrorNotify {
    int32 error_code = 1;
    string error_msg = 2;
}
```

## 数据模型

### UserInfo
用户信息
```protobuf
message UserInfo {
    int32 user_id = 1;
    string username = 2;
    string nickname = 3;
    string avatar_url = 4;
    int32 win_count = 5;
    int32 lose_count = 6;
    int32 draw_count = 7;
    int32 rating = 8;
    int32 total_games = 9;
}
```

### RoomInfo
房间信息
```protobuf
message RoomInfo {
    string room_id = 1;
    string room_name = 2;
    int32 owner_id = 3;
    bool has_password = 4;
    RoomState state = 5;
    UserInfo player1 = 6;
    UserInfo player2 = 7;
    int32 spectator_count = 8;
}
```

### Move
棋步
```protobuf
message Move {
    int32 row = 1;
    int32 col = 2;
    int32 player_id = 3;
}
```

### RankEntry
排行榜条目
```protobuf
message RankEntry {
    int32 rank = 1;
    UserInfo user_info = 2;
    int32 value = 3;
}
```

## 连接管理

### 连接建立
1. 客户端连接服务器
2. 服务器接受连接
3. 客户端发送登录或注册请求
4. 服务器验证并返回结果

### 连接保持
- 客户端定期发送心跳包（建议间隔30秒）
- 服务器超时时间：60秒
- 超时后自动断开连接

### 连接断开
1. 正常断开：发送登出请求
2. 异常断开：网络中断、超时等
3. 服务器清理：清除用户在线状态、退出房间等

## 错误处理

### 错误码格式
所有错误消息都包含错误码和错误描述：
```protobuf
message ErrorNotify {
    int32 error_code = 1;
    string error_msg = 2;
}
```

### 通用错误码
- `0`: 成功
- `1`: 未知错误
- `2`: 参数错误
- `3`: 权限不足
- `4`: 资源不存在
- `5`: 资源已存在
- `6`: 操作失败
- `7`: 服务器内部错误

## 安全性

### 认证
- 使用Token进行身份验证
- Token有效期：24小时
- 敏感操作需要验证Token

### 数据加密
- 密码使用bcrypt加密存储
- 通信使用TCP，建议在生产环境使用TLS

### 防护措施
- 限制单个IP的连接数
- 限制消息发送频率
- 防止SQL注入
- 防止XSS攻击

## 性能优化

### 消息批量处理
- 支持批量发送多个消息
- 减少网络往返次数

### 缓存策略
- 用户信息缓存
- 房间列表缓存
- 排行榜缓存

### 连接池
- MySQL连接池
- Redis连接池

## 服务器配置

### 配置参数
```ini
# 服务器端口
port=8888

# MySQL配置
mysql_host=localhost
mysql_port=3306
mysql_user=gomoku
mysql_password=****
mysql_database=gomoku_db

# Redis配置
redis_host=localhost
redis_port=6379

# 日志配置
log_level=INFO
log_file=logs/server.log
```

### 性能调优
- 调整线程池大小
- 调整缓冲区大小
- 调整超时时间

## 调试

### 日志级别
- `DEBUG`: 详细调试信息
- `INFO`: 一般信息
- `WARN`: 警告信息
- `ERROR`: 错误信息

### 常见问题
1. 连接失败：检查网络、端口、防火墙
2. 认证失败：检查用户名、密码、Token
3. 消息解析失败：检查Protobuf版本、消息格式
4. 性能问题：检查服务器负载、网络延迟

## 版本信息

- API版本：1.0
- Protobuf版本：3.21.12
- 最后更新：2026-03-11

## 联系方式

如有问题，请提交GitHub Issue：
https://github.com/Tiange-source/AI-project/issues