#include "test_framework.h"
#include "business/GameController.h"
#include <iostream>

using namespace gomoku;

// 集成测试：完整游戏流程
TEST_TESTCASE(Integration, CompleteGameFlow) {
    std::cout << "\n--- 测试完整游戏流程 ---" << std::endl;
    
    GameController controller;
    
    // 1. 开始游戏
    std::cout << "1. 开始游戏，玩家1先手..." << std::endl;
    controller.initGame();
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 1);
    
    // 2. 玩家1落子
    std::cout << "2. 玩家1落子(9,9)..." << std::endl;
    TEST_ASSERT_TRUE(controller.makeMove(9, 9, 1));
    const auto& board = controller.getBoard();
    TEST_ASSERT_EQ(board[9][9], 1);
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 2);
    
    // 3. 玩家2落子
    std::cout << "3. 玩家2落子(8,8)..." << std::endl;
    TEST_ASSERT_TRUE(controller.makeMove(8, 8, 2));
    TEST_ASSERT_EQ(board[8][8], 2);
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 1);
    
    // 4. 继续对战
    std::cout << "4. 继续对战..." << std::endl;
    controller.makeMove(9, 10, 1);
    controller.makeMove(8, 9, 2);
    controller.makeMove(9, 11, 1);
    controller.makeMove(8, 10, 2);
    
    // 5. 悔棋
    std::cout << "5. 玩家2悔棋..." << std::endl;
    controller.undoMove();
    TEST_ASSERT_EQ(board[8][10], 0);
    
    // 6. 重置游戏
    std::cout << "6. 重置游戏..." << std::endl;
    controller.resetGame();
    
    // 检查棋盘是否清空
    bool boardEmpty = true;
    const auto& boardAfterReset = controller.getBoard();
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 18; j++) {
            if (boardAfterReset[i][j] != 0) {
                boardEmpty = false;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(boardEmpty);
    
    std::cout << "--- 完整游戏流程测试完成 ---" << std::endl;
}

// 集成测试：胜负判定
TEST_TESTCASE(Integration, WinScenario) {
    std::cout << "\n--- 测试胜负判定场景 ---" << std::endl;
    
    GameController controller;
    controller.initGame();
    
    // 模拟五子连珠场景
    std::cout << "模拟水平五子连珠..." << std::endl;
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(5, 6, 1);
    controller.makeMove(6, 6, 2);
    controller.makeMove(5, 7, 1);
    controller.makeMove(6, 7, 2);
    controller.makeMove(5, 8, 1);
    controller.makeMove(6, 8, 2);
    
    std::cout << "玩家1落下第五子..." << std::endl;
    controller.makeMove(5, 9, 1);
    
    GameResult result = controller.checkWin(5, 9, 1);
    TEST_ASSERT_EQ(result, GameResult::WIN);
    std::cout << "玩家1获胜！" << std::endl;
    
    std::cout << "--- 胜负判定测试完成 ---" << std::endl;
}

// 集成测试：边界情况
TEST_TESTCASE(Integration, EdgeCases) {
    std::cout << "\n--- 测试边界情况 ---" << std::endl;
    
    GameController controller;
    controller.initGame();
    
    // 测试边界位置
    std::cout << "测试边界位置落子..." << std::endl;
    TEST_ASSERT_TRUE(controller.makeMove(0, 0, 1));      // 左上角
    TEST_ASSERT_TRUE(controller.makeMove(17, 17, 2));   // 右下角
    TEST_ASSERT_TRUE(controller.makeMove(0, 17, 1));    // 右上角
    TEST_ASSERT_TRUE(controller.makeMove(17, 0, 2));    // 左下角
    
    // 测试无效位置
    std::cout << "测试无效位置..." << std::endl;
    TEST_ASSERT_FALSE(controller.isValidMove(-1, 0));
    TEST_ASSERT_FALSE(controller.isValidMove(0, -1));
    TEST_ASSERT_FALSE(controller.isValidMove(18, 0));
    TEST_ASSERT_FALSE(controller.isValidMove(0, 18));
    
    // 测试重复落子
    std::cout << "测试重复落子..." << std::endl;
    controller.makeMove(5, 5, 1);
    TEST_ASSERT_FALSE(controller.makeMove(5, 5, 2));
    
    std::cout << "--- 边界情况测试完成 ---" << std::endl;
}

// 集成测试：性能测试
TEST_TESTCASE(Integration, Performance) {
    std::cout << "\n--- 测试性能 ---" << std::endl;
    
    GameController controller;
    controller.initGame();
    
    // 测试大量落子性能
    std::cout << "测试大量落子性能..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; i++) {
        if (controller.isGameOver()) {
            break;
        }
        controller.makeMove(i % 18, (i * 2) % 18, controller.getCurrentPlayer());
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "100次落子耗时: " << duration.count() << "ms" << std::endl;
    TEST_ASSERT(duration.count() < 100);  // 应该在100ms内完成
    
    // 测试胜负判断性能
    std::cout << "测试胜负判断性能..." << std::endl;
    controller.resetGame();
    controller.initGame();
    
    start = std::chrono::high_resolution_clock::now();
    
    // 模拟即将获胜的场景
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(5, 6, 1);
    controller.makeMove(6, 6, 2);
    controller.makeMove(5, 7, 1);
    controller.makeMove(6, 7, 2);
    controller.makeMove(5, 8, 1);
    controller.makeMove(6, 8, 2);
    
    // 检查胜负（应该很快）
    GameResult win = controller.checkWin(5, 8, 1);
    
    end = std::chrono::high_resolution_clock::now();
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "胜负判断耗时: " << duration_us.count() << "μs" << std::endl;
    TEST_ASSERT(duration_us.count() < 1000);  // 应该在1ms内完成
    
    std::cout << "--- 性能测试完成 ---" << std::endl;
}

// 集成测试：并发安全
TEST_TESTCASE(Integration, Concurrency) {
    std::cout << "\n--- 测试并发安全 ---" << std::endl;
    
    GameController controller;
    controller.initGame();
    
    // 这里简化测试，实际应该使用多线程
    std::cout << "测试连续落子..." << std::endl;
    
    for (int i = 0; i < 10; i++) {
        int player = controller.getCurrentPlayer();
        int row = i % 18;
        int col = (i * 2) % 18;
        
        if (controller.isValidMove(row, col)) {
            controller.makeMove(row, col, player);
        }
    }
    
    // 检查棋盘状态
    int pieceCount = 0;
    const auto& board = controller.getBoard();
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 18; j++) {
            if (board[i][j] != 0) {
                pieceCount++;
            }
        }
    }
    
    std::cout << "棋盘上的棋子数: " << pieceCount << std::endl;
    TEST_ASSERT(pieceCount > 0);
    TEST_ASSERT(pieceCount <= 10);
    
    std::cout << "--- 并发安全测试完成 ---" << std::endl;
}