# 网络联机五子棋客户端

基于Qt 5.15+和C++11开发的网络联机五子棋客户端应用程序。

## 项目信息

- **版本**: 1.0.0
- **开发环境**: Qt 5.15.2, C++11
- **通信协议**: Protobuf
- **操作系统**: Windows

## 项目结构

```
GomokuClient/
├── GomokuClient.pro          # Qt项目文件
├── README.md                 # 项目说明文档
├── src/                      # 源代码目录
│   ├── main.cpp              # 程序入口
│   ├── mainwindow.h/cpp      # 主窗口
│   ├── mainwindow.ui         # UI设计器文件
│   ├── network/              # 网络通信层
│   │   ├── tcpclient.h/cpp
│   │   ├── protobufcodec.h/cpp
│   │   └── messagedispatcher.h/cpp
│   ├── data/                 # 数据层
│   │   ├── userprofile.h/cpp
│   │   └── roominfo.h/cpp
│   ├── ui/                   # UI层（待实现）
│   └── logic/                # 逻辑层（待实现）
└── build/                    # 编译输出目录
```

## 开发进度

### 已完成 (Issue #1)
- ✅ 网络通信层和Protobuf编解码
  - TcpClient类：连接管理、消息收发
  - ProtobufCodec类：序列化/反序列化，处理粘包/半包
  - MessageDispatcher类：消息分发

### 已完成 (Issue #2 - 部分完成)
- ✅ UserProfile类：用户信息管理、Token本地保存
- ✅ RoomInfo类：房间信息管理
- ⏳ 登录界面（待实现）
- ⏳ 注册界面（待实现）

### 待实现
- ⏳ Issue #3: 游戏大厅界面
- ⏳ Issue #4: 游戏房间界面
- ⏳ Issue #5: 游戏逻辑和AI引擎
- ⏳ Issue #6: 聊天功能
- ⏳ Issue #7: 观战功能
- ⏳ Issue #8: 排行榜界面

## 编译说明

### 前置要求

1. **Qt 5.15+**
   - 下载地址: https://www.qt.io/download
   - 推荐版本: Qt 5.15.2

2. **Protobuf 3.0+**
   - 下载地址: https://github.com/protocolbuffers/protobuf/releases
   - 需要protoc编译器

3. **C++11兼容的编译器**
   - MSVC 2015+
   - MinGW 4.8+

### 编译步骤

1. **编译Protobuf协议文件**

   首先需要编译共享的Protobuf定义文件：

   ```bash
   cd ../shared/proto
   ./compile.sh
   ```

   注意：在Windows上，如果没有bash环境，需要手动运行protoc命令：

   ```bash
   protoc --proto_path=. --cpp_out=../build gomoku.proto
   ```

2. **打开项目**

   使用Qt Creator打开 `GomokuClient.pro` 文件。

3. **配置项目**

   - 选择编译套件（Kit）
   - 确认Protobuf头文件路径正确

4. **编译项目**

   - 在Qt Creator中点击"构建"按钮
   - 或使用命令行：`qmake && make`

### 编译输出

编译成功后，可执行文件位于：
- Debug模式: `build/debug/GomokuClient.exe`
- Release模式: `build/release/GomokuClient.exe`

## 运行说明

### 首次运行

1. 确保服务器正在运行
2. 启动客户端应用程序
3. 点击"游戏"菜单 → "连接服务器"
4. 输入服务器地址和端口
5. 登录或注册账号

### 配置说明

用户信息和Token保存在本地配置文件中：
- Windows: `HKEY_CURRENT_USER\Software\GomokuClient\User`

## 协议说明

### 消息格式

```
[4字节长度][4字节类型][消息体]
```

### 消息类型

详细的协议定义请参考 `../shared/proto/gomoku.proto` 文件。

主要消息类型：
- 认证相关: LOGIN, REGISTER, LOGOUT
- 房间相关: CREATE_ROOM, JOIN_ROOM, LEAVE_ROOM, ROOM_LIST
- 游戏相关: START_GAME, MOVE, GAME_OVER, UNDO
- 匹配相关: RANDOM_MATCH, MATCH_SUCCESS
- 聊天相关: CHAT_MESSAGE
- 观战相关: SPECTATE
- 排行榜: RANK_LIST

## 开发规范

### 编码规范
- 所有源文件使用UTF-8编码
- Git提交消息使用UTF-8编码
- 类名使用大驼峰命名（PascalCase）
- 成员变量使用小驼峰命名（camelCase）
- 私有成员变量前加下划线前缀

### Git提交规范
- 每完成一个Issue提交一次代码
- 提交消息格式：`Issue #X: 描述`
- 例如：`Issue #1: 实现网络通信层和Protobuf编解码`

## 常见问题

### Q: 编译时提示找不到Protobuf头文件
A: 确保已经编译了 `../shared/proto/gomoku.proto` 文件，并且 `../shared/build` 目录下有生成的 `.pb.h` 和 `.pb.cc` 文件。

### Q: 运行时提示连接失败
A: 确保服务器正在运行，并且服务器地址和端口配置正确。

### Q: 中文显示乱码
A: 确保所有源文件使用UTF-8编码保存，并在Qt Creator中设置默认编码为UTF-8。

## 联系方式

- GitHub: https://github.com/Tiange-source/AI-project
- Issues: https://github.com/Tiange-source/AI-project/issues

## 许可证

本项目仅供学习交流使用。