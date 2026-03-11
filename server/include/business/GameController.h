#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include <string>
#include <vector>
#include <mutex>

namespace gomoku {

struct Move {
    int row;
    int col;
    int playerId;
};

enum class GameResult {
    CONTINUE,
    WIN,
    LOSE,
    DRAW
};

class GameController {
public:
    static const int BOARD_SIZE = 18;
    static const int WIN_COUNT = 5;
    
    GameController();
    ~GameController();
    
    // 初始化游戏
    void initGame();
    
    // 落子
    bool makeMove(int row, int col, int playerId);
    
    // 检查落子是否合法
    bool isValidMove(int row, int col) const;
    
    // 检查胜负
    GameResult checkWin(int row, int col, int playerId) const;
    
    // 获取当前棋盘
    const std::vector<std::vector<int>>& getBoard() const;
    
    // 获取当前玩家
    int getCurrentPlayer() const;
    
    // 切换玩家
    void switchPlayer();
    
    // 获取落子历史
    const std::vector<Move>& getMoveHistory() const;
    
    // 悔棋
    bool undoMove();
    
    // 重置游戏
    void resetGame();
    
    // 获取游戏是否结束
    bool isGameOver() const;
    
    // 获取获胜者
    int getWinner() const;

private:
    // 检查四个方向是否连珠
    bool checkDirection(int row, int col, int playerId, int dRow, int dCol) const;
    
    // 统计某一方向的连续棋子数
    int countConsecutive(int row, int col, int playerId, int dRow, int dCol) const;

    std::vector<std::vector<int>> board_;
    int currentPlayer_;
    std::vector<Move> moveHistory_;
    bool gameOver_;
    int winner_;
    std::mutex mutex_;
};

} // namespace gomoku

#endif // GAME_CONTROLLER_H