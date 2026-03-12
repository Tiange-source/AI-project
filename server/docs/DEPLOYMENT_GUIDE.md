# 五子棋服务器部署指南

## 1. 环境准备

### 1.1 系统要求
- **操作系统**：Linux（Ubuntu 20.04+推荐）
- **CPU**：2核及以上
- **内存**：4GB及以上
- **磁盘**：20GB及以上
- **网络**：公网IP或内网可访问

### 1.2 软件依赖
- **GCC**：4.8+
- **CMake**：3.10+
- **Protobuf**：3.21.12
- **MySQL**：5.7+
- **Redis**：5.0+
- **pthread**：标准库

### 1.3 安装依赖

#### Ubuntu/Debian
```bash
# 更新软件包列表
sudo apt-get update

# 安装基础构建工具
sudo apt-get install -y build-essential cmake git

# 安装Protobuf
sudo apt-get install -y libprotobuf-dev protobuf-compiler

# 安装MySQL客户端
sudo apt-get install -y libmysqlclient-dev

# 安装Redis客户端
sudo apt-get install -y libhiredis-dev

# 安装MySQL服务器（可选，也可使用远程MySQL）
sudo apt-get install -y mysql-server

# 安装Redis服务器（可选，也可使用远程Redis）
sudo apt-get install -y redis-server
```

#### CentOS/RHEL
```bash
# 安装基础构建工具
sudo yum groupinstall -y "Development Tools"
sudo yum install -y cmake git

# 安装Protobuf
sudo yum install -y protobuf-devel protobuf-compiler

# 安装MySQL客户端
sudo yum install -y mysql-devel

# 安装Redis客户端
sudo yum install -y hiredis-devel

# 安装MySQL服务器（可选）
sudo yum install -y mysql-server

# 安装Redis服务器（可选）
sudo yum install -y redis
```

## 2. 数据库配置

### 2.1 MySQL配置

#### 启动MySQL
```bash
# Ubuntu/Debian
sudo systemctl start mysql
sudo systemctl enable mysql

# CentOS/RHEL
sudo systemctl start mysqld
sudo systemctl enable mysqld
```

#### 创建数据库和用户
```bash
# 登录MySQL
mysql -u root -p

# 在MySQL命令行中执行
CREATE DATABASE gomoku_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

# 创建用户
CREATE USER 'gomoku'@'localhost' IDENTIFIED BY 'your_strong_password';
CREATE USER 'gomoku'@'%' IDENTIFIED BY 'your_strong_password';

# 授予权限
GRANT ALL PRIVILEGES ON gomoku_db.* TO 'gomoku'@'localhost';
GRANT ALL PRIVILEGES ON gomoku_db.* TO 'gomoku'@'%';

# 刷新权限
FLUSH PRIVILEGES;

# 退出MySQL
EXIT;
```

#### 导入初始数据
```bash
# 导入数据库初始化脚本
mysql -u gomoku -p gomoku_db < /root/ai/server/sql/init_database.sql
```

#### MySQL配置优化
编辑 `/etc/mysql/mysql.conf.d/mysqld.cnf`：
```ini
[mysqld]
# 连接配置
max_connections = 500
max_user_connections = 400

# 缓存配置
innodb_buffer_pool_size = 1G
query_cache_size = 64M
query_cache_type = 1

# 字符集
character-set-server = utf8mb4
collation-server = utf8mb4_unicode_ci

# 日志
slow_query_log = 1
slow_query_log_file = /var/log/mysql/slow-query.log
long_query_time = 2
```

重启MySQL：
```bash
sudo systemctl restart mysql
```

### 2.2 Redis配置

#### 启动Redis
```bash
# Ubuntu/Debian
sudo systemctl start redis-server
sudo systemctl enable redis-server

# CentOS/RHEL
sudo systemctl start redis
sudo systemctl enable redis
```

#### 配置Redis
编辑 `/etc/redis/redis.conf`：
```ini
# 网络配置
bind 0.0.0.0
port 6379

# 密码配置（生产环境强烈建议设置）
requirepass your_redis_password

# 内存配置
maxmemory 2gb
maxmemory-policy allkeys-lru

# 持久化配置
save 900 1
save 300 10
save 60 10000

# 日志
loglevel notice
logfile /var/log/redis/redis-server.log
```

重启Redis：
```bash
sudo systemctl restart redis-server
```

#### 测试Redis连接
```bash
redis-cli
# 如果设置了密码
redis-cli -a your_redis_password

# 在Redis命令行中
AUTH your_redis_password
PING
# 应返回 PONG
```

## 3. 编译部署

### 3.1 克隆代码仓库
```bash
# 克隆仓库（如果还没有）
git clone git@github.com:Tiange-source/AI-project.git
cd AI-project
git checkout server
```

### 3.2 编译Protobuf
```bash
cd /root/ai/shared/proto
./compile.sh

# 验证Protobuf生成
ls -la ../build/
# 应该看到 gomoku.pb.h 和 gomoku.pb.cc
```

### 3.3 编译服务器
```bash
cd /root/ai/server

# 创建构建目录
mkdir -p build
cd build

# 配置CMake（Debug模式）
cmake .. -DCMAKE_BUILD_TYPE=Debug

# 编译
make -j$(nproc)

# 验证可执行文件
ls -la bin/
# 应该看到 gomoku_server
```

### 3.4 Release模式编译（生产环境）
```bash
# 清理旧的构建
cd /root/ai/server
rm -rf build
mkdir build
cd build

# 配置CMake（Release模式）
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)
```

### 3.5 编译测试（可选）
```bash
cd /root/ai/server/tests
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### 3.6 运行测试
```bash
cd /root/ai/server/tests/build

# 运行单元测试
./bin/test_buffer
./bin/test_game_controller

# 运行集成测试
./bin/test_integration

# 运行所有测试
make run_all_tests
```

## 4. 配置服务器

### 4.1 创建配置文件目录
```bash
cd /root/ai/server
mkdir -p config logs
```

### 4.2 修改配置文件
编辑 `/root/ai/server/config/server.conf`：

```ini
# ========================================
# 服务器配置
# ========================================
server.port=8888
server.ip=0.0.0.0
server.thread_pool_size=4

# ========================================
# MySQL配置
# ========================================
mysql_host=localhost
mysql_port=3306
mysql_database=gomoku_db
mysql_user=gomoku
mysql_password=your_strong_password
mysql_connection_pool_size=10

# ========================================
# Redis配置
# ========================================
redis_host=127.0.0.1
redis_port=6379
redis_password=your_redis_password
redis_db_index=0

# ========================================
# 日志配置
# ========================================
log.level=INFO
log.file_path=logs/gomoku_server.log
log.max_file_size=100MB
log.max_files=10

# ========================================
# 游戏配置
# ========================================
game.match_timeout=300
game.move_timeout=60
game.max_spectators=10
game.room_cleanup_interval=3600

# ========================================
# 聊天配置
# ========================================
chat.max_message_length=200
chat.max_history_size=100
chat.sensitive_words_filter=true

# ========================================
# 心跳配置
# ========================================
heartbeat.interval=30
heartbeat.timeout=90
```

### 4.3 创建日志目录
```bash
cd /root/ai/server
mkdir -p logs
chmod 755 logs
```

## 5. 启动服务器

### 5.1 前台启动（测试用）
```bash
cd /root/ai/server/build
./bin/gomoku_server -c ../config/server.conf

# 按Ctrl+C停止服务器
```

### 5.2 后台启动（生产用）
```bash
cd /root/ai/server/build
nohup ./bin/gomoku_server -c ../config/server.conf > ../logs/server.log 2>&1 &

# 查看进程ID
echo $!

# 查看日志
tail -f ../logs/gomoku_server.log
```

### 5.3 使用systemd管理（推荐）

#### 创建服务文件
创建 `/etc/systemd/system/gomoku-server.service`：

```ini
[Unit]
Description=Gomoku Game Server
After=network.target mysql.service redis.service
Requires=network.target

[Service]
Type=simple
User=root
WorkingDirectory=/root/ai/server/build
ExecStart=/root/ai/server/build/bin/gomoku_server -c /root/ai/server/config/server.conf
Restart=on-failure
RestartSec=10
LimitNOFILE=65536

# 环境变量
Environment="LD_LIBRARY_PATH=/usr/local/lib"

# 标准输出和错误输出
StandardOutput=append:/root/ai/server/logs/gomoku_server.log
StandardError=append:/root/ai/server/logs/gomoku_server.log

[Install]
WantedBy=multi-user.target
```

#### 启动服务
```bash
# 重新加载systemd配置
sudo systemctl daemon-reload

# 启动服务
sudo systemctl start gomoku-server

# 设置开机自启
sudo systemctl enable gomoku-server

# 查看服务状态
sudo systemctl status gomoku-server

# 查看日志
sudo journalctl -u gomoku-server -f
```

#### 管理命令
```bash
# 启动服务
sudo systemctl start gomoku-server

# 停止服务
sudo systemctl stop gomoku-server

# 重启服务
sudo systemctl restart gomoku-server

# 查看状态
sudo systemctl status gomoku-server

# 查看日志
sudo journalctl -u gomoku-server -n 100
sudo journalctl -u gomoku-server -f
```

## 6. 验证部署

### 6.1 检查服务状态
```bash
# 检查进程
ps aux | grep gomoku_server

# 检查端口
netstat -tlnp | grep 8888
# 或
ss -tlnp | grep 8888

# 检查日志
tail -f /root/ai/server/logs/gomoku_server.log
```

### 6.2 运行测试
```bash
cd /root/ai/server/tests/build
./bin/test_buffer
./bin/test_game_controller
./bin/test_integration
```

### 6.3 连接测试
```bash
# 使用提供的测试脚本
cd /root/ai/server
python3 test_connection.py

# 手动测试
telnet localhost 8888
```

### 6.4 数据库连接测试
```bash
# MySQL连接测试
mysql -u gomoku -p -h localhost gomoku_db

# Redis连接测试
redis-cli -a your_redis_password
PING
```

## 7. 监控和维护

### 7.1 日志监控
```bash
# 实时查看日志
tail -f /root/ai/server/logs/gomoku_server.log

# 查看错误日志
grep ERROR /root/ai/server/logs/gomoku_server.log

# 查看警告日志
grep WARN /root/ai/server/logs/gomoku_server.log

# 查看最近的日志
tail -n 100 /root/ai/server/logs/gomoku_server.log
```

### 7.2 性能监控
```bash
# 查看CPU和内存使用
top -p $(pgrep gomoku_server)

# 或使用htop
htop -p $(pgrep gomoku_server)

# 查看网络连接
netstat -anp | grep gomoku_server
ss -anp | grep gomoku_server

# 查看文件描述符
ls -la /proc/$(pgrep gomoku_server)/fd | wc -l
```

### 7.3 数据库监控
```bash
# MySQL连接数
mysql -u gomoku -p -e "SHOW PROCESSLIST;"
mysql -u gomoku -p -e "SHOW STATUS LIKE 'Threads_connected';"

# Redis连接数
redis-cli -a your_redis_password INFO clients

# Redis内存使用
redis-cli -a your_redis_password INFO memory
```

### 7.4 系统资源监控
```bash
# 查看磁盘使用
df -h

# 查看内存使用
free -h

# 查看CPU使用
top

# 查看系统负载
uptime
```

## 8. 常见问题

### 8.1 端口被占用
```bash
# 查找占用进程
sudo lsof -i :8888

# 修改配置文件中的端口号
# 或停止占用端口的进程
sudo kill -9 <PID>
```

### 8.2 数据库连接失败
```bash
# 检查MySQL服务状态
sudo systemctl status mysql

# 检查MySQL用户权限
mysql -u gomoku -p -e "SELECT USER();"

# 测试MySQL连接
mysql -u gomoku -p -h localhost gomoku_db

# 检查防火墙
sudo ufw status
sudo ufw allow 3306
```

### 8.3 Redis连接失败
```bash
# 检查Redis服务状态
sudo systemctl status redis-server

# 测试Redis连接
redis-cli -a your_redis_password PING

# 检查防火墙
sudo ufw allow 6379
```

### 8.4 内存不足
```bash
# 增加swap空间
sudo fallocate -l 2G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile

# 永久启用
echo '/swapfile none swap sw 0 0' | sudo tee -a /etc/fstab
```

### 8.5 权限问题
```bash
# 修改文件权限
sudo chown -R root:root /root/ai/server
sudo chmod -R 755 /root/ai/server

# 修改日志目录权限
sudo chmod 777 /root/ai/server/logs
```

## 9. 备份和恢复

### 9.1 数据库备份
```bash
# 备份MySQL
mysqldump -u gomoku -p gomoku_db > backup_$(date +%Y%m%d_%H%M%S).sql

# 备份MySQL（压缩）
mysqldump -u gomoku -p gomoku_db | gzip > backup_$(date +%Y%m%d_%H%M%S).sql.gz

# 自动备份脚本
#!/bin/bash
BACKUP_DIR="/root/backups"
mkdir -p $BACKUP_DIR
mysqldump -u gomoku -p'your_password' gomoku_db | gzip > $BACKUP_DIR/mysql_$(date +%Y%m%d_%H%M%S).sql.gz
find $BACKUP_DIR -name "mysql_*.sql.gz" -mtime +7 -delete
```

### 9.2 Redis备份
```bash
# 手动保存
redis-cli -a your_redis_password BGSAVE

# 复制RDB文件
cp /var/lib/redis/dump.rdb /root/backups/redis_$(date +%Y%m%d_%H%M%S).rdb

# 自动备份脚本
#!/bin/bash
BACKUP_DIR="/root/backups"
mkdir -p $BACKUP_DIR
redis-cli -a your_redis_password BGSAVE
sleep 5
cp /var/lib/redis/dump.rdb $BACKUP_DIR/redis_$(date +%Y%m%d_%H%M%S).rdb
find $BACKUP_DIR -name "redis_*.rdb" -mtime +7 -delete
```

### 9.3 数据恢复
```bash
# 恢复MySQL
mysql -u gomoku -p gomoku_db < backup_20260312_120000.sql

# 恢复MySQL（压缩）
gunzip < backup_20260312_120000.sql.gz | mysql -u gomoku -p gomoku_db

# 恢复Redis
redis-cli -a your_redis_password SHUTDOWN
cp /root/backups/redis_20260312_120000.rdb /var/lib/redis/dump.rdb
redis-server /etc/redis/redis.conf
```

## 10. 升级部署

### 10.1 备份当前版本
```bash
# 备份可执行文件
cp /root/ai/server/build/bin/gomoku_server /root/backup/gomoku_server.old

# 备份配置文件
cp /root/ai/server/config/server.conf /root/backup/server.conf.old

# 备份数据库
mysqldump -u gomoku -p gomoku_db > /root/backup/mysql_backup.sql
```

### 10.2 部署新版本
```bash
# 拉取最新代码
cd /root/ai
git pull origin server

# 重新编译
cd server/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 重启服务
sudo systemctl restart gomoku-server
```

### 10.3 回滚
```bash
# 停止服务
sudo systemctl stop gomoku-server

# 恢复旧版本
cp /root/backup/gomoku_server.old /root/ai/server/build/bin/gomoku_server

# 重启服务
sudo systemctl start gomoku-server
```

## 11. 性能优化建议

### 11.1 系统参数优化
编辑 `/etc/sysctl.conf`：
```ini
# 文件描述符限制
fs.file-max = 1000000

# 网络参数
net.core.somaxconn = 32768
net.ipv4.tcp_max_syn_backlog = 32768
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30

# 内存参数
vm.swappiness = 10
```

应用配置：
```bash
sudo sysctl -p
```

### 11.2 文件描述符限制
编辑 `/etc/security/limits.conf`：
```
* soft nofile 65536
* hard nofile 65536
```

### 11.3 MySQL优化
```ini
[mysqld]
# 连接配置
max_connections = 500
max_user_connections = 400

# 缓存配置
innodb_buffer_pool_size = 2G
innodb_log_file_size = 256M
innodb_flush_log_at_trx_commit = 2

# 查询缓存
query_cache_size = 128M
query_cache_type = 1

# 慢查询日志
slow_query_log = 1
long_query_time = 2
```

### 11.4 Redis优化
```ini
# 内存配置
maxmemory 4gb
maxmemory-policy allkeys-lru

# 持久化配置
save 900 1
save 300 10
save 60 10000

# 网络配置
tcp-keepalive 300
```

## 12. 安全建议

### 12.1 防火墙配置
```bash
# Ubuntu/Debian
sudo ufw allow 8888/tcp
sudo ufw allow 22/tcp
sudo ufw enable

# CentOS/RHEL
sudo firewall-cmd --permanent --add-port=8888/tcp
sudo firewall-cmd --permanent --add-port=22/tcp
sudo firewall-cmd --reload
```

### 12.2 数据库安全
```bash
# 限制MySQL访问
# 编辑 /etc/mysql/mysql.conf.d/mysqld.cnf
bind-address = 127.0.0.1

# 设置强密码
# 定期更换密码

# 限制用户权限
# 只授予必要的权限
```

### 12.3 Redis安全
```bash
# 设置密码
# 编辑 /etc/redis/redis.conf
requirepass your_strong_password

# 禁用危险命令
# 编辑 /etc/redis/redis.conf
rename-command FLUSHDB ""
rename-command FLUSHALL ""
rename-command CONFIG ""
```

### 12.4 系统安全
```bash
# 定期更新系统
sudo apt-get update
sudo apt-get upgrade

# 安装安全工具
sudo apt-get install fail2ban
sudo systemctl enable fail2ban

# 配置SSH
# 编辑 /etc/ssh/sshd_config
PermitRootLogin no
PasswordAuthentication no
```

## 13. 故障排查

### 13.1 服务器无法启动
```bash
# 查看日志
sudo journalctl -u gomoku-server -n 100

# 检查配置文件
cat /root/ai/server/config/server.conf

# 检查端口占用
sudo lsof -i :8888

# 检查数据库连接
mysql -u gomoku -p -h localhost gomoku_db
redis-cli -a your_redis_password PING
```

### 13.2 性能问题
```bash
# 查看CPU使用
top -p $(pgrep gomoku_server)

# 查看内存使用
free -h

# 查看网络连接
netstat -anp | grep gomoku_server

# 查看数据库性能
mysql -u gomoku -p -e "SHOW PROCESSLIST;"
redis-cli -a your_redis_password INFO
```

### 13.3 内存泄漏
```bash
# 使用valgrind检测
valgrind --leak-check=full ./bin/gomoku_server

# 监控内存使用
watch -n 1 'ps aux | grep gomoku_server'
```

## 14. 联系和支持

- **项目地址**：https://github.com/Tiange-source/AI-project
- **Issue追踪**：https://github.com/Tiange-source/AI-project/issues
- **分支**：server
- **版本**：1.0.0

---

**文档版本**：1.0.0  
**最后更新**：2026-03-12  
**维护者**：iFlow CLI