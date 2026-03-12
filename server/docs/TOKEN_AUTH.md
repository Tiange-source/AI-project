# Token认证机制说明

## 概述

服务端使用Token机制进行用户身份验证，客户端在登录成功后会收到服务器返回的Token，后续所有需要身份验证的操作都需要携带此Token。

## Token生成流程

### 1. 用户登录

客户端发送`LOGIN_REQUEST`：

```protobuf
message LoginRequest {
    string username = "username";
    string password = "password";
}
```

### 2. 服务器验证

服务器验证用户名和密码后，生成Token并返回：

```protobuf
message LoginResponse {
    bool success = true;
    string token = "abc123...";  // 生成的Token
    UserInfo user_info = {...};
}
```

### 3. Token生成算法

Token采用以下规则生成：
- 格式：随机字符串（32位）
- 存储：Redis缓存
- 有效期：24小时（可根据配置调整）
- 绑定：与用户ID绑定

## Token使用

### 登出请求

客户端需要携带Token进行登出：

```protobuf
message LogoutRequest {
    string token = "abc123...";  // 必须携带登录时返回的Token
}
```

### 其他需要Token的操作

以下操作需要携带Token（在消息体或连接上下文中）：
- LEAVE_ROOM_REQUEST
- CREATE_ROOM_REQUEST
- JOIN_ROOM_REQUEST
- CHAT_MESSAGE_REQUEST
- 等等...

**注意**：当前实现中，Token主要通过连接上下文管理。客户端登录成功后，服务器会将用户ID和Token的映射关系保存在内存和Redis中。后续操作通过连接ID关联到用户信息，无需每次都显式发送Token。

## Token验证

### 服务器端验证

服务器在处理需要身份验证的操作时，会执行以下验证：

1. **内存验证**：检查内存中的Token映射
2. **Redis验证**：检查Redis中的Token映射

```cpp
bool UserManager::isTokenValid(int userId, const std::string& token) {
    // 1. 检查内存缓存
    auto it = userTokens_.find(userId);
    if (it != userTokens_.end()) {
        return it->second == token;
    }

    // 2. 检查Redis缓存
    std::ostringstream key;
    key << "user:token:" << token;
    std::string userIdStr = redis_->get(key.str());
    return !userIdStr.empty();
}
```

## Token生命周期

### Token生成

- **触发时机**：用户登录成功
- **生成位置**：`UserManager::generateToken()`
- **存储位置**：
  - 内存：`userTokens_[userId] = token`
  - Redis：`user:token:{token} -> userId`

### Token使用

- **用途**：验证用户身份
- **有效期**：24小时
- **自动刷新**：暂不支持（需要重新登录）

### Token销毁

- **触发时机**：
  - 用户主动登出
  - Token过期
  - 服务器重启（内存中的Token丢失，但Redis中的Token仍有效）

```cpp
void UserManager::removeToken(int userId) {
    // 从内存移除
    userTokens_.erase(userId);

    // 从Redis移除
    std::ostringstream key;
    key << "user:token:" << token;
    redis_->del(key.str());
}
```

## 客户端实现建议

### 1. Token存储

客户端应该在登录成功后保存Token：

```cpp
// 伪代码示例
void onLoginSuccess(LoginResponse response) {
    if (response.success()) {
        // 保存Token到本地配置或内存
        saveToken(response.token());

        // 保存用户信息
        saveUserInfo(response.user_info());

        // 登录成功，跳转到大厅
        navigateToLobby();
    }
}
```

### 2. Token使用

在发送需要身份验证的请求时，确保已登录：

```cpp
// 伪代码示例
void createRoom(const string& roomName, const string& password) {
    // 检查是否已登录
    if (!isLoggedIn()) {
        showError("请先登录");
        return;
    }

    // 构建请求
    CreateRoomRequest request;
    request.set_room_name(roomName);
    request.set_password(password);

    // 发送请求（无需显式携带Token，服务器通过连接识别用户）
    sendRequest(CREATE_ROOM_REQUEST, request);
}
```

### 3. Token过期处理

当收到Token无效的错误时，应提示用户重新登录：

```cpp
void onError(ErrorNotify error) {
    if (error.error_code() == INVALID_TOKEN) {
        showWarning("登录已过期，请重新登录");
        logout();
        navigateToLogin();
    }
}
```

## 常见问题

### Q1: Token有效期多久？

**A**: 当前设置为24小时，但可以通过修改代码调整。建议在服务器配置文件中添加Token过期时间配置。

### Q2: Token可以自动续期吗？

**A**: 当前不支持自动续期。用户需要重新登录获取新Token。如果需要支持自动续期，可以考虑实现Token刷新机制。

### Q3: Token丢失怎么办？

**A**: 如果Token丢失（如服务器重启），用户需要重新登录。建议在Redis中持久化Token，或者实现Token持久化机制。

### Q4: 如何提高Token安全性？

**A**: 可以考虑以下改进：
1. 使用JWT（JSON Web Token）格式
2. 添加Token签名验证
3. 设置Token过期时间
4. 实现Token刷新机制
5. 支持Token撤销

### Q5: 多设备登录如何处理？

**A**: 当前实现中，同一用户的多设备登录会产生多个Token，都是有效的。如果需要限制单设备登录，可以：
1. 存储用户的Token列表
2. 新登录时使旧Token失效
3. 只允许最新的Token有效

## API参考

### 登录接口

```
Request: LOGIN_REQUEST (1)
Response: LOGIN_RESPONSE (2)
```

**登录成功响应示例**：
```protobuf
LoginResponse {
    success: true
    token: "a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6"
    user_info {
        user_id: 123
        username: "testuser"
        nickname: "测试用户"
        // ... 其他字段
    }
}
```

### 登出接口

```
Request: LOGOUT_REQUEST (5)
Response: LOGOUT_RESPONSE (6)
```

## 服务器日志

服务器会记录Token相关操作：

```
[INFO] UserManager::login - user logged in: 123, token: a1b2c3...
[WARN] UserManager::logout - invalid token for user ID: 123
[INFO] UserManager::addOnlineUser - user 123 added to online list
```

## 联系方式

如有问题，请提交GitHub Issue或查看服务端日志。

---

**文档版本**: 1.0
**最后更新**: 2026-03-12
**维护者**: 服务端团队