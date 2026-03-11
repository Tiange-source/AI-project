# 服务端构建指南

## 环境要求

### 系统要求
- Linux操作系统（推荐Ubuntu 20.04+）
- GCC 4.8+ 或 Clang 3.3+
- CMake 3.10+

### 依赖库
- Protobuf 3.21.12
- MySQL客户端库 (libmysqlclient)
- Redis客户端库 (hiredis)
- pthreads（线程库）

### 安装依赖

#### Ubuntu/Debian
```bash
# 安装编译工具
sudo apt-get update
sudo apt-get install -y build-essential cmake

# 安装Protobuf
sudo apt-get install -y libprotobuf-dev protobuf-compiler

# 安装MySQL客户端库
sudo apt-get install -y libmysqlclient-dev

# 安装Redis客户端库
sudo apt-get install -y libhiredis-dev

# 验证安装
protoc --version  # 应该显示 libprotoc 3.21.12
```

#### CentOS/RHEL
```bash
# 安装编译工具
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake

# 安装Protobuf
sudo yum install -y protobuf-devel protobuf-compiler

# 安装MySQL客户端库
sudo yum install -y mysql-devel

# 安装Redis客户端库
sudo yum install -y hiredis-devel
```

## 构建步骤

### 1. 克隆仓库
```bash
git clone git@github.com:Tiange-source/AI-project.git
cd AI-project
git checkout server
```

### 2. 编译Protobuf协议
```bash
cd shared/proto
./compile.sh
cd ../..
```

### 3. 配置CMake
```bash
cd server
mkdir -p build
cd build
cmake ..
```

### 4. 编译
```bash
make -j$(nproc)
```

### 5. 安装（可选）
```bash
sudo make install
```

## 配置服务器

### 1. 创建配置文件
复制配置文件模板并修改：
```bash
cp config/server.conf /etc/gomoku_server/server.conf
```

### 2. 修改配置参数
编辑 `/etc/gomoku_server/server.conf`：

```ini
# 服务器端口
port=8888

# MySQL配置
mysql_host=localhost
mysql_port=3306
mysql_user=gomoku
mysql_password=your_password
mysql_database=gomoku_db

# Redis配置
redis_host=localhost
redis_port=6379
```

### 3. 初始化数据库
```bash
mysql -u root -p < sql/init_database.sql
```

## 运行服务器

### 前台运行
```bash
cd build
./bin/gomoku_server
```

### 后台运行
```bash
cd build
nohup ./bin/gomoku_server -c /etc/gomoku_server/server.conf > logs/server.log 2>&1 &
```

### 指定配置文件
```bash
./bin/gomoku_server -c /path/to/server.conf
```

### 停止服务器
```bash
# 查找进程ID
ps aux | grep gomoku_server

# 停止进程
kill <PID>

# 或使用Ctrl+C（前台运行）
```

## 日志

### 日志文件位置
- 默认：`logs/server.log`
- 日志级别：DEBUG、INFO、WARN、ERROR

### 查看日志
```bash
# 实时查看日志
tail -f logs/server.log

# 查看最近100行
tail -n 100 logs/server.log

# 查看错误日志
grep ERROR logs/server.log
```

## 自检脚本

运行自检脚本验证环境：
```bash
cd server
bash scripts/self_check.sh
```

自检脚本会检查：
- 目录结构完整性
- Protobuf版本
- 依赖库安装
- Git仓库状态
- CMake配置
- 代码统计

## 常见问题

### 1. Protobuf版本不匹配
**问题**: 编译时提示protobuf版本不兼容

**解决**:
```bash
# 检查当前版本
protoc --version

# 如果不是3.21.12，重新安装
sudo apt-get remove protobuf-compiler libprotobuf-dev
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.21.12/protoc-3.21.12-linux-x86_64.zip
unzip protoc-3.21.12-linux-x86_64.zip
sudo cp bin/protoc /usr/local/bin/
sudo chmod +x /usr/local/bin/protoc
```

### 2. MySQL连接失败
**问题**: 服务器启动时无法连接MySQL

**解决**:
- 检查MySQL服务是否运行：`sudo systemctl status mysql`
- 检查配置文件中的MySQL连接参数
- 检查MySQL用户权限：`GRANT ALL PRIVILEGES ON gomoku_db.* TO 'gomoku'@'localhost';`

### 3. Redis连接失败
**问题**: 服务器启动时无法连接Redis

**解决**:
- 检查Redis服务是否运行：`sudo systemctl status redis`
- 检查配置文件中的Redis连接参数
- 检查Redis配置是否允许远程连接

### 4. 编译错误
**问题**: CMake配置或编译失败

**解决**:
```bash
# 清理构建目录
rm -rf build
mkdir build
cd build

# 重新配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 查看详细编译信息
make VERBOSE=1
```

### 5. 端口被占用
**问题**: 服务器启动失败，提示端口已被占用

**解决**:
```bash
# 查找占用端口的进程
sudo lsof -i :8888

# 停止占用端口的进程
sudo kill <PID>

# 或修改配置文件中的端口号
```

## 性能优化

### 1. 编译优化
```bash
# Release模式编译
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 2. 系统参数优化
编辑 `/etc/sysctl.conf`：
```ini
# 增加最大文件描述符数量
fs.file-max = 1000000

# 增加TCP连接队列长度
net.core.somaxconn = 32768
net.ipv4.tcp_max_syn_backlog = 32768

# 启用TCP快速打开
net.ipv4.tcp_fastopen = 3
```

应用配置：
```bash
sudo sysctl -p
```

### 3. Redis配置优化
编辑Redis配置文件：
```ini
# 最大内存限制
maxmemory 2gb

# 内存淘汰策略
maxmemory-policy allkeys-lru

# 持久化配置
save 900 1
save 300 10
save 60 10000
```

## 开发调试

### 使用GDB调试
```bash
# 编译Debug版本
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# 使用GDB运行
gdb ./bin/gomoku_server

# GDB常用命令
(gdb) break main          # 设置断点
(gdb) run                 # 运行程序
(gdb) next                # 单步执行
(gdb) print variable_name # 打印变量
(gdb) backtrace           # 查看调用栈
(gdb) quit                # 退出GDB
```

### 使用Valgrind检测内存泄漏
```bash
# 安装Valgrind
sudo apt-get install valgrind

# 运行检测
valgrind --leak-check=full --show-leak-kinds=all ./bin/gomoku_server
```

## 部署建议

### 生产环境部署

1. **使用系统服务**
   创建 `/etc/systemd/system/gomoku-server.service`：
   ```ini
   [Unit]
   Description=Gomoku Game Server
   After=network.target mysql.service redis.service

   [Service]
   Type=simple
   User=gomoku
   Group=gomoku
   WorkingDirectory=/opt/gomoku_server
   ExecStart=/opt/gomoku_server/bin/gomoku_server -c /etc/gomoku_server/server.conf
   Restart=on-failure
   RestartSec=10
   LimitNOFILE=65536

   [Install]
   WantedBy=multi-user.target
   ```

   启动服务：
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl start gomoku-server
   sudo systemctl enable gomoku-server
   ```

2. **使用反向代理**
   使用Nginx作为反向代理：
   ```nginx
   server {
       listen 80;
       server_name your-domain.com;

       location / {
           proxy_pass http://localhost:8888;
           proxy_http_version 1.1;
           proxy_set_header Upgrade $http_upgrade;
           proxy_set_header Connection "upgrade";
           proxy_set_header Host $host;
           proxy_set_header X-Real-IP $remote_addr;
           proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
       }
   }
   ```

3. **监控和日志**
   - 使用Prometheus + Grafana监控服务器性能
   - 使用ELK Stack收集和分析日志
   - 配置日志轮转防止磁盘占满

## 联系方式

如有问题，请提交GitHub Issue：
https://github.com/Tiange-source/AI-project/issues