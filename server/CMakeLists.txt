cmake_minimum_required(VERSION 3.10)
project(gomoku_server VERSION 1.0.0 LANGUAGES CXX)

# C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 查找依赖包
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 查找MySQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

# 查找Redis (hiredis)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)

if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
    message(FATAL_ERROR "hiredis not found. Please install hiredis development package.")
endif()

# Protobuf编译
set(PROTO_SRC_DIR ${CMAKE_SOURCE_DIR}/../shared/proto)
set(PROTO_BUILD_DIR ${CMAKE_BINARY_DIR})

# 复制proto文件到构建目录
file(COPY ${PROTO_SRC_DIR}/gomoku.proto DESTINATION ${CMAKE_BINARY_DIR})

# 编译proto文件
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${CMAKE_BINARY_DIR}/gomoku.proto)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${MYSQL_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}  # protobuf生成的头文件目录
)

# 链接目录
link_directories(
    ${MYSQL_LIBRARY_DIRS}
)

# 源文件
set(SERVER_SRCS
    src/main.cpp
    src/server/GameServer.cpp
    src/network/Buffer.cpp
    src/network/Channel.cpp
    src/network/EventLoop.cpp
    src/network/Poller.cpp
    src/network/Socket.cpp
    src/network/Acceptor.cpp
    src/network/TcpConnection.cpp
    src/storage/MySQLClient.cpp
    src/storage/RedisClient.cpp
    src/business/UserManager.cpp
    src/business/RoomManager.cpp
    src/business/GameController.cpp
    src/business/MatchManager.cpp
    src/business/ChatManager.cpp
    src/business/SpectatorManager.cpp
    src/protocol/ProtobufCodec.cpp
    src/protocol/MessageDispatcher.cpp
    src/utils/Logger.cpp
    src/utils/ThreadPool.cpp
)

# 添加Protobuf生成的源文件
list(APPEND SERVER_SRCS ${PROTO_SRCS})

# 创建可执行文件
add_executable(gomoku_server ${SERVER_SRCS})

# 链接库
target_link_libraries(gomoku_server
    Threads::Threads
    protobuf::libprotobuf
    ${MYSQL_LIBRARIES}
    ${HIREDIS_LIBRARY}
)

# 安装规则
install(TARGETS gomoku_server DESTINATION bin)
install(DIRECTORY config/ DESTINATION etc/gomoku_server)
install(FILES README.md DESTINATION .)

# 打印配置信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf version: ${Protobuf_VERSION}")
message(STATUS "MySQL libraries: ${MYSQL_LIBRARIES}")
message(STATUS "hiredis library: ${HIREDIS_LIBRARY}")