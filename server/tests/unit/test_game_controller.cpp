#include "test_framework.h"
#include "business/GameController.h"
#include <iostream>

// Disable logger for tests
#define LOG_INFO(msg) ((void)0)
#define LOG_WARN(msg) ((void)0)
#define LOG_ERROR(msg) ((void)0)
#define LOG_DEBUG(msg) ((void)0)

using namespace gomoku;

// GameController测试套件
TEST_TESTCASE(GameController, Initialization) {
    GameController controller;
    controller.initGame();
    
    // 检查棋盘初始化为空
    const auto& board = controller.getBoard();
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 18; j++) {
            TEST_ASSERT_EQ(board[i][j], 0);
        }
    }
}

TEST_TESTCASE(GameController, InitGame) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 1);  // 玩家1先手
}

TEST_TESTCASE(GameController, ValidMove) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_TRUE(controller.isValidMove(9, 9));
    TEST_ASSERT_TRUE(controller.makeMove(9, 9, 1));
}

TEST_TESTCASE(GameController, InvalidMove_OutOfBounds) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_FALSE(controller.isValidMove(-1, 0));
    TEST_ASSERT_FALSE(controller.isValidMove(18, 0));
    TEST_ASSERT_FALSE(controller.isValidMove(0, -1));
    TEST_ASSERT_FALSE(controller.isValidMove(0, 18));
}

TEST_TESTCASE(GameController, InvalidMove_Occupied) {
    GameController controller;
    controller.initGame();
    
    controller.makeMove(5, 5, 1);
    TEST_ASSERT_FALSE(controller.isValidMove(5, 5));
}

TEST_TESTCASE(GameController, InvalidMove_NotYourTurn) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_FALSE(controller.makeMove(5, 5, 2));  // 玩家2不能先手
}

TEST_TESTCASE(GameController, SwitchPlayer) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 1);
    controller.makeMove(9, 9, 1);
    TEST_ASSERT_EQ(controller.getCurrentPlayer(), 2);
}

TEST_TESTCASE(GameController, NoWin_ThreeInRow) {
    GameController controller;
    controller.initGame();
    
    // 水平三子
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(5, 6, 1);
    controller.makeMove(5, 7, 2);
    
    GameResult result = controller.checkWin(5, 6, 1);
    TEST_ASSERT_EQ(result, GameResult::CONTINUE);
}

TEST_TESTCASE(GameController, Win_FiveInRow_Horizontal) {
    GameController controller;
    controller.initGame();
    
    // 水平五子连珠
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(5, 6, 1);
    controller.makeMove(6, 6, 2);
    controller.makeMove(5, 7, 1);
    controller.makeMove(6, 7, 2);
    controller.makeMove(5, 8, 1);
    controller.makeMove(6, 8, 2);
    controller.makeMove(5, 9, 1);  // 第五子
    
    GameResult result = controller.checkWin(5, 9, 1);
    TEST_ASSERT_EQ(result, GameResult::WIN);
}

TEST_TESTCASE(GameController, Win_FiveInRow_Vertical) {
    GameController controller;
    controller.initGame();
    
    // 垂直五子连珠
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 6, 2);
    controller.makeMove(6, 5, 1);
    controller.makeMove(7, 6, 2);
    controller.makeMove(7, 5, 1);
    controller.makeMove(8, 6, 2);
    controller.makeMove(8, 5, 1);
    controller.makeMove(9, 6, 2);
    controller.makeMove(9, 5, 1);  // 第五子
    
    GameResult result = controller.checkWin(9, 5, 1);
    TEST_ASSERT_EQ(result, GameResult::WIN);
}

TEST_TESTCASE(GameController, Win_FiveInRow_Diagonal) {
    GameController controller;
    controller.initGame();
    
    // 对角线五子连珠
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(6, 6, 1);
    controller.makeMove(7, 5, 2);
    controller.makeMove(7, 7, 1);
    controller.makeMove(8, 5, 2);
    controller.makeMove(8, 8, 1);
    controller.makeMove(9, 5, 2);
    controller.makeMove(9, 9, 1);  // 第五子
    
    GameResult result = controller.checkWin(9, 9, 1);
    TEST_ASSERT_EQ(result, GameResult::WIN);
}

TEST_TESTCASE(GameController, Win_FiveInRow_AntiDiagonal) {
    GameController controller;
    controller.initGame();
    
    // 反对角线五子连珠
    controller.makeMove(5, 9, 1);
    controller.makeMove(6, 9, 2);
    controller.makeMove(6, 8, 1);
    controller.makeMove(7, 9, 2);
    controller.makeMove(7, 7, 1);
    controller.makeMove(8, 9, 2);
    controller.makeMove(8, 6, 1);
    controller.makeMove(9, 9, 2);
    controller.makeMove(9, 5, 1);  // 第五子
    
    GameResult result = controller.checkWin(9, 5, 1);
    TEST_ASSERT_EQ(result, GameResult::WIN);
}

TEST_TESTCASE(GameController, UndoMove) {
    GameController controller;
    controller.initGame();
    
    controller.makeMove(9, 9, 1);
    controller.makeMove(8, 8, 2);
    
    const auto& board = controller.getBoard();
    TEST_ASSERT_NE(board[8][8], 0);
    
    controller.undoMove();
    
    TEST_ASSERT_NE(board[9][9], 0);  // 第一步还在
    TEST_ASSERT_EQ(board[8][8], 0);   // 第二步已撤销
}

TEST_TESTCASE(GameController, ResetGame) {
    GameController controller;
    controller.initGame();
    
    controller.makeMove(9, 9, 1);
    controller.makeMove(8, 8, 2);
    
    controller.resetGame();
    
    // 检查棋盘是否清空
    const auto& board = controller.getBoard();
    for (int i = 0; i < 18; i++) {
        for (int j = 0; j < 18; j++) {
            TEST_ASSERT_EQ(board[i][j], 0);
        }
    }
}

TEST_TESTCASE(GameController, MoveHistory) {
    GameController controller;
    controller.initGame();
    
    controller.makeMove(9, 9, 1);
    controller.makeMove(8, 8, 2);
    
    const auto& history = controller.getMoveHistory();
    TEST_ASSERT_EQ(history.size(), 2);
    TEST_ASSERT_EQ(history[0].row, 9);
    TEST_ASSERT_EQ(history[0].col, 9);
    TEST_ASSERT_EQ(history[0].playerId, 1);
    TEST_ASSERT_EQ(history[1].row, 8);
    TEST_ASSERT_EQ(history[1].col, 8);
    TEST_ASSERT_EQ(history[1].playerId, 2);
}

TEST_TESTCASE(GameController, GameState) {
    GameController controller;
    controller.initGame();
    
    TEST_ASSERT_FALSE(controller.isGameOver());
    
    controller.makeMove(5, 5, 1);
    controller.makeMove(6, 5, 2);
    controller.makeMove(5, 6, 1);
    controller.makeMove(6, 6, 2);
    controller.makeMove(5, 7, 1);
    controller.makeMove(6, 7, 2);
    controller.makeMove(5, 8, 1);
    controller.makeMove(6, 8, 2);
    controller.makeMove(5, 9, 1);
    
    // 五子连珠，游戏应该结束
    TEST_ASSERT_TRUE(controller.isGameOver());
    TEST_ASSERT_EQ(controller.getWinner(), 1);
}