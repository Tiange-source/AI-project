#!/bin/bash

# Protobuf编译脚本
# 用于编译gomoku.proto文件生成C++代码

PROTO_FILE="gomoku.proto"
PROTO_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${PROTO_DIR}/../build"

# 颜色输出
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Protobuf 编译脚本${NC}"
echo -e "${YELLOW}========================================${NC}"

# 检查protoc是否安装
if ! command -v protoc &> /dev/null; then
    echo -e "${RED}错误: protoc 未安装${NC}"
    echo "请先安装 Protocol Buffers 编译器"
    echo "Ubuntu/Debian: sudo apt-get install protobuf-compiler"
    echo "CentOS/RHEL: sudo yum install protobuf-compiler"
    echo "macOS: brew install protobuf"
    exit 1
fi

# 检查protobuf文件是否存在
if [ ! -f "${PROTO_DIR}/${PROTO_FILE}" ]; then
    echo -e "${RED}错误: ${PROTO_FILE} 文件不存在${NC}"
    echo "当前目录: ${PROTO_DIR}"
    exit 1
fi

# 创建输出目录
mkdir -p "${OUTPUT_DIR}"

# 编译protobuf文件
echo -e "${YELLOW}正在编译 ${PROTO_FILE}...${NC}"

protoc \
    --proto_path="${PROTO_DIR}" \
    --cpp_out="${OUTPUT_DIR}" \
    "${PROTO_DIR}/${PROTO_FILE}"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ 编译成功！${NC}"
    echo -e "输出目录: ${OUTPUT_DIR}"
    echo ""
    echo "生成的文件:"
    ls -lh "${OUTPUT_DIR}"/*.pb.h "${OUTPUT_DIR}"/*.pb.cc 2>/dev/null || echo "未找到生成的文件"
else
    echo -e "${RED}✗ 编译失败！${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  编译完成${NC}"
echo -e "${GREEN}========================================${NC}"