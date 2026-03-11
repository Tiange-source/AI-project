#!/bin/bash
# 测试编译和运行脚本

set -e

echo "========================================"
echo "  Gomoku Server Test Runner"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 进入测试目录
cd "$(dirname "$0")"
TEST_DIR=$(pwd)
BUILD_DIR="$TEST_DIR/build"

# 检查Protobuf是否编译
if [ ! -f "$TEST_DIR/../../shared/build/gomoku.pb.h" ]; then
    echo -e "${YELLOW}[WARNING]${NC} Protobuf未编译，正在编译..."
    cd "$TEST_DIR/../../shared/proto"
    ./compile.sh
    cd "$TEST_DIR"
fi

# 创建构建目录
echo "1. 创建构建目录..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 配置CMake
echo "2. 配置CMake..."
if [ ! -f "Makefile" ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

# 编译
echo "3. 编译测试..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${NC} 编译失败"
    exit 1
fi

echo -e "${GREEN}[SUCCESS]${NC} 编译成功"
echo ""

# 运行测试
echo "4. 运行测试..."
echo ""

# 运行单元测试
echo "--- 单元测试 ---"
echo ""

if [ -f "bin/test_buffer" ]; then
    echo "运行Buffer测试..."
    ./bin/test_buffer
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[OK]${NC} Buffer测试通过"
    else
        echo -e "${RED}[FAIL]${NC} Buffer测试失败"
    fi
    echo ""
fi

if [ -f "bin/test_game_controller" ]; then
    echo "运行GameController测试..."
    ./bin/test_game_controller
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[OK]${NC} GameController测试通过"
    else
        echo -e "${RED}[FAIL]${NC} GameController测试失败"
    fi
    echo ""
fi

# 运行集成测试
if [ -f "bin/test_integration" ]; then
    echo "--- 集成测试 ---"
    echo ""
    echo "运行集成测试..."
    ./bin/test_integration
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}[OK]${NC} 集成测试通过"
    else
        echo -e "${RED}[FAIL]${NC} 集成测试失败"
    fi
    echo ""
fi

# 运行所有测试（CMake）
echo "========================================"
echo "  运行CMake测试"
echo "========================================"
cd "$BUILD_DIR"
ctest --output-on-failure

echo ""
echo "========================================"
echo "  测试完成"
echo "========================================"
echo ""
echo "测试目录: $BUILD_DIR"
echo "可执行文件: $BUILD_DIR/bin/"