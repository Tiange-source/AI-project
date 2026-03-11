# 网络联机五子棋对战系统

基于C/S架构的网络联机五子棋对战系统，支持单机AI对战、在线玩家对战、观战系统等功能。

## 技术栈

- **服务端**: C++11, Reactor网络模型, Protobuf, MySQL, Redis
- **客户端**: Qt 5.15+, C++
- **通信协议**: Protobuf

## 项目结构

```
/root/ai/
├── server/              # 服务端代码（提交到server分支）
│   ├── proto/          # Protobuf定义文件（从shared同步）
│   ├── src/
│   │   ├── network/    # Reactor网络层
│   │   ├── business/   # 业务逻辑层
│   │   ├── storage/    # 存储层（Redis/MySQL）
│   │   └── utils/      # 工具类
│   ├── include/        # 头文件
│   ├── build/          # 编译输出
│   ├── tests/          # 单元测试
│   └── docs/           # 服务器端文档
├── client/             # 客户端代码（提交到client分支）
└── shared/             # 共享资源（提交到main分支）
    ├── proto/          # 共享的Protobuf文件
    └── docs/           # 共享文档
```

## 分支说明

- **main**: 共享资源（Protobuf协议、文档等）
- **server**: 服务端代码
- **client**: 客户端代码

## Protobuf协议

Protobuf协议定义在 `shared/proto/gomoku.proto` 中。

### 编译Protobuf

```bash
cd shared/proto
./compile.sh
```

生成的文件将保存到 `shared/build/` 目录：
- `gomoku.pb.h` - C++头文件
- `gomoku.pb.cc` - C++实现文件

### 消息类型

系统支持以下消息类型：

- **认证相关**: LOGIN, REGISTER, LOGOUT
- **房间相关**: CREATE_ROOM, JOIN_ROOM, LEAVE_ROOM, ROOM_LIST
- **游戏相关**: START_GAME, MOVE, GAME_OVER, UNDO
- **匹配相关**: RANDOM_MATCH, MATCH_SUCCESS
- **聊天相关**: CHAT_MESSAGE
- **观战相关**: SPECTATE
- **排行榜**: RANK_LIST
- **心跳和错误**: HEARTBEAT, ERROR_NOTIFY

详细的协议定义请参考 `shared/proto/gomoku.proto` 文件。

## 开发环境

### 依赖

- GCC 4.8+ (支持C++11)
- CMake 3.10+
- Protobuf 3.0+
- MySQL 5.7+
- Redis 5.0+
- Qt 5.15+ (客户端)

### 安装依赖 (Ubuntu/Debian)

```bash
# C++编译器
sudo apt-get install build-essential g++

# CMake
sudo apt-get install cmake

# Protobuf
sudo apt-get install protobuf-compiler libprotobuf-dev

# MySQL
sudo apt-get install mysql-server libmysqlclient-dev

# Redis
sudo apt-get install redis-server libhiredis-dev

# Qt (客户端)
sudo apt-get install qt5-default qtbase5-dev
```

## 数据库初始化

数据库初始化脚本位于 `server/sql/init_database.sql`。

```bash
mysql -u root -p < server/sql/init_database.sql
```

## 构建服务端

```bash
cd server
mkdir build && cd build
cmake ..
make
```

## 运行服务端

```bash
cd server/build
./gomoku_server
```

## 开发文档

详细的开发文档请参考：
- [网络联机五子棋开发文档.md](./网络联机五子棋开发文档.md)
- [服务端部署文档](./server/docs/DEPLOYMENT.md)
- [API文档](./server/docs/API.md)
- [客户端开发指南](./shared/docs/CLIENT_DEV_GUIDE.md)

## 协作方式

项目使用GitHub Issues进行协作沟通。

### 提交Issue

使用 `gh` 命令行工具提交Issue：

```bash
gh issue create --repo Tiange-source/AI-project --title "Issue标题" --body "Issue描述"
```

### 查看Issues

```bash
gh issue list --repo Tiange-source/AI-project
```

## 许可证

本项目仅供学习交流使用。

## 联系方式

- GitHub: https://github.com/Tiange-source/AI-project
- Issues: https://github.com/Tiange-source/AI-project/issues