cmake_minimum_required(VERSION 3.10)
project(gomoku_tests VERSION 1.0.0 LANGUAGES CXX)

# C++11标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g -O0")

# 输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找依赖包
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)

# 查找MySQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

# 查找Redis (hiredis)
find_path(HIREDIS_INCLUDE_DIR hiredis/hiredis.h)
find_library(HIREDIS_LIBRARY NAMES hiredis)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/../include
    ${CMAKE_SOURCE_DIR}/../../shared/build
    ${CMAKE_SOURCE_DIR}/../../build
    ${MYSQL_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIR}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
)

# 链接目录
link_directories(
    ${MYSQL_LIBRARY_DIRS}
)

# 测试框架源文件
set(TEST_FRAMEWORK_SRCS
    test_framework.h
)

# 单元测试源文件
set(UNIT_TEST_SRCS
    unit/test_buffer.cpp
    unit/test_game_controller.cpp
)

# 集成测试源文件
set(INTEGRATION_TEST_SRCS
    integration/test_integration.cpp
)

# 添加主程序
set(TEST_MAIN_SRCS
    test_main.cpp
)

# 创建单元测试可执行文件
add_executable(test_buffer
    ${TEST_MAIN_SRCS}
    ${TEST_FRAMEWORK_SRCS}
    unit/test_buffer.cpp
)

add_executable(test_game_controller
    ${TEST_MAIN_SRCS}
    ${TEST_FRAMEWORK_SRCS}
    unit/test_game_controller.cpp
)

# 创建集成测试可执行文件
add_executable(test_integration
    ${TEST_MAIN_SRCS}
    ${TEST_FRAMEWORK_SRCS}
    integration/test_integration.cpp
)

# 链接库
target_link_libraries(test_buffer
    Threads::Threads
    protobuf::libprotobuf
)

target_link_libraries(test_game_controller
    Threads::Threads
    protobuf::libprotobuf
)

target_link_libraries(test_integration
    Threads::Threads
    protobuf::libprotobuf
)

# 添加实现文件
target_sources(test_buffer PRIVATE
    ${CMAKE_SOURCE_DIR}/../src/network/Buffer.cpp
)

target_sources(test_game_controller PRIVATE
    ${CMAKE_SOURCE_DIR}/../src/business/GameController.cpp
    ${CMAKE_SOURCE_DIR}/../src/utils/Logger.cpp
)

target_sources(test_integration PRIVATE
    ${CMAKE_SOURCE_DIR}/../src/business/GameController.cpp
    ${CMAKE_SOURCE_DIR}/../src/network/Buffer.cpp
    ${CMAKE_SOURCE_DIR}/../src/utils/Logger.cpp
)

# 添加自定义目标
add_custom_target(run_all_tests
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_buffer
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_game_controller
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_integration
    DEPENDS test_buffer test_game_controller test_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all tests..."
)

# 添加测试到CTest
enable_testing()
add_test(NAME BufferTest COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_buffer)
add_test(NAME GameControllerTest COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_game_controller)
add_test(NAME IntegrationTest COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_integration)

# 打印配置信息
message(STATUS "Test Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Protobuf Version: ${Protobuf_VERSION}")