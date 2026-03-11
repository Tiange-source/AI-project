#!/bin/bash
# 服务端代码自检脚本

echo "========================================"
echo "  Gomoku Server Self-Check"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 错误计数
ERRORS=0
WARNINGS=0

# 检查函数
check_file() {
    local file=$1
    if [ -f "$file" ]; then
        echo -e "${GREEN}[OK]${NC} File exists: $file"
    else
        echo -e "${RED}[ERROR]${NC} File missing: $file"
        ((ERRORS++))
    fi
}

# 检查目录结构
echo "1. 检查目录结构..."
check_file "server/include/network/EventLoop.h"
check_file "server/include/network/Poller.h"
check_file "server/include/network/Channel.h"
check_file "server/include/network/Socket.h"
check_file "server/include/network/Acceptor.h"
check_file "server/include/network/TcpConnection.h"
check_file "server/include/network/Buffer.h"
check_file "server/include/storage/MySQLClient.h"
check_file "server/include/storage/RedisClient.h"
check_file "server/include/business/UserManager.h"
check_file "server/include/business/RoomManager.h"
check_file "server/include/business/GameController.h"
check_file "server/include/business/MatchManager.h"
check_file "server/include/business/ChatManager.h"
check_file "server/include/business/SpectatorManager.h"
check_file "server/include/protocol/ProtobufCodec.h"
check_file "server/include/protocol/MessageDispatcher.h"
check_file "server/include/server/GameServer.h"
check_file "server/include/utils/Logger.h"
check_file "server/include/utils/ThreadPool.h"

check_file "server/src/network/EventLoop.cpp"
check_file "server/src/network/Poller.cpp"
check_file "server/src/network/Channel.cpp"
check_file "server/src/network/Socket.cpp"
check_file "server/src/network/Acceptor.cpp"
check_file "server/src/network/TcpConnection.cpp"
check_file "server/src/network/Buffer.cpp"
check_file "server/src/storage/MySQLClient.cpp"
check_file "server/src/storage/RedisClient.cpp"
check_file "server/src/business/UserManager.cpp"
check_file "server/src/business/RoomManager.cpp"
check_file "server/src/business/GameController.cpp"
check_file "server/src/business/MatchManager.cpp"
check_file "server/src/business/ChatManager.cpp"
check_file "server/src/business/SpectatorManager.cpp"
check_file "server/src/protocol/ProtobufCodec.cpp"
check_file "server/src/protocol/MessageDispatcher.cpp"
check_file "server/src/server/GameServer.cpp"
check_file "server/src/main.cpp"
check_file "server/src/utils/Logger.cpp"
check_file "server/src/utils/ThreadPool.cpp"

echo ""

# 检查共享资源
echo "2. 检查共享资源..."
check_file "shared/proto/gomoku.proto"
check_file "shared/proto/compile.sh"
check_file "shared/build/gomoku.pb.h"
check_file "shared/build/gomoku.pb.cc"
echo ""

# 检查配置文件
echo "3. 检查配置文件..."
check_file "server/CMakeLists.txt"
check_file "server/config/server.conf"
check_file "server/sql/init_database.sql"
echo ""

# 检查protobuf版本
echo "4. 检查protobuf版本..."
if command -v protoc &> /dev/null; then
    PROTOC_VERSION=$(protoc --version | awk '{print $2}')
    echo -e "${GREEN}[OK]${NC} Protobuf version: $PROTOC_VERSION"
    
    # 检查版本是否 >= 3.21.12
    if [ "$PROTOC_VERSION" = "3.21.12" ]; then
        echo -e "${GREEN}[OK]${NC} Protobuf version matches required version 3.21.12"
    else
        echo -e "${YELLOW}[WARNING]${NC} Protobuf version $PROTOC_VERSION, required: 3.21.12"
        ((WARNINGS++))
    fi
else
    echo -e "${RED}[ERROR]${NC} protoc command not found"
    ((ERRORS++))
fi
echo ""

# 检查依赖库
echo "5. 检查依赖库..."
if pkg-config --exists mysqlclient; then
    echo -e "${GREEN}[OK]${NC} MySQL client library found"
else
    echo -e "${RED}[ERROR]${NC} MySQL client library not found"
    ((ERRORS++))
fi

if [ -f "/usr/include/hiredis/hiredis.h" ] || [ -f "/usr/local/include/hiredis/hiredis.h" ]; then
    echo -e "${GREEN}[OK]${NC} Hiredis library found"
else
    echo -e "${RED}[ERROR]${NC} Hiredis library not found"
    ((ERRORS++))
fi

if pkg-config --exists protobuf; then
    echo -e "${GREEN}[OK]${NC} Protobuf library found"
else
    echo -e "${RED}[ERROR]${NC} Protobuf library not found"
    ((ERRORS++))
fi

if pkg-config --exists threads; then
    echo -e "${GREEN}[OK]${NC} Threads library found"
else
    echo -e "${YELLOW}[WARNING]${NC} Threads library check skipped"
fi
echo ""

# 检查Git仓库
echo "6. 检查Git仓库..."
if [ -d ".git" ]; then
    echo -e "${GREEN}[OK]${NC} Git repository initialized"
    
    # 检查分支
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    echo "Current branch: $CURRENT_BRANCH"
    
    # 检查远程仓库
    if git remote -v | grep -q "github.com"; then
        echo -e "${GREEN}[OK]${NC} Remote repository configured"
    else
        echo -e "${YELLOW}[WARNING]${NC} No remote repository configured"
        ((WARNINGS++))
    fi
else
    echo -e "${RED}[ERROR]${NC} Not a git repository"
    ((ERRORS++))
fi
echo ""

# 检查编译配置
echo "7. 检查CMakeLists.txt..."
if [ -f "server/CMakeLists.txt" ]; then
    echo -e "${GREEN}[OK]${NC} CMakeLists.txt exists"
    
    # 检查CMake版本要求
    if grep -q "cmake_minimum_required(VERSION 3.10)" server/CMakeLists.txt; then
        echo -e "${GREEN}[OK]${NC} CMake version requirement found"
    else
        echo -e "${YELLOW}[WARNING]${NC} CMake version requirement may be missing"
        ((WARNINGS++))
    fi
    
    # 检查C++标准
    if grep -q "CMAKE_CXX_STANDARD 11" server/CMakeLists.txt; then
        echo -e "${GREEN}[OK]${NC} C++11 standard configured"
    else
        echo -e "${YELLOW}[WARNING]${NC} C++11 standard may not be configured"
        ((WARNINGS++))
    fi
else
    echo -e "${RED}[ERROR]${NC} CMakeLists.txt not found"
    ((ERRORS++))
fi
echo ""

# 代码质量检查
echo "8. 代码质量检查..."
echo "Checking for common issues..."

# 检查TODO注释
TODO_COUNT=$(grep -r "TODO" server/include server/src --include="*.h" --include="*.cpp" | wc -l)
if [ $TODO_COUNT -gt 0 ]; then
    echo -e "${YELLOW}[INFO]${NC} Found $TODO_COUNT TODO comments"
fi

# 检查FIXME注释
FIXME_COUNT=$(grep -r "FIXME" server/include server/src --include="*.h" --include="*.cpp" | wc -l)
if [ $FIXME_COUNT -gt 0 ]; then
    echo -e "${YELLOW}[WARNING]${NC} Found $FIXME_COUNT FIXME comments"
    ((WARNINGS++))
fi
echo ""

# 统计信息
echo "9. 代码统计..."
if command -v cloc &> /dev/null; then
    echo "Using cloc for code statistics..."
    cloc server/include server/src --quiet
else
    echo "C++ files count:"
    echo "  Headers: $(find server/include -name "*.h" | wc -l)"
    echo "  Sources: $(find server/src -name "*.cpp" | wc -l)"
    echo ""
    echo "Total lines of C++ code:"
    echo "  Headers: $(find server/include -name "*.h" -exec cat {} \; | wc -l)"
    echo "  Sources: $(find server/src -name "*.cpp" -exec cat {} \; | wc -l)"
fi
echo ""

# 总结
echo "========================================"
echo "  Self-Check Summary"
echo "========================================"
if [ $ERRORS -eq 0 ] && [ $WARNINGS -eq 0 ]; then
    echo -e "${GREEN}[SUCCESS]${NC} All checks passed!"
    exit 0
elif [ $ERRORS -eq 0 ]; then
    echo -e "${YELLOW}[WARNING]${NC} Checks passed with $WARNINGS warnings"
    exit 0
else
    echo -e "${RED}[ERROR]${NC} Checks failed with $ERRORS errors and $WARNINGS warnings"
    exit 1
fi