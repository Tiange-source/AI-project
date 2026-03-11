#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QObject>
#include <QVector>

/**
 * @brief 游戏状态枚举
 */
enum GameState {
    IDLE = 0,       // 空闲
    PLAYING = 1,    // 游戏中
    WIN = 2,        // 胜利
    LOSE = 3,       // 失败
    DRAW = 4        // 平局
};

/**
 * @brief 棋步结构
 */
struct Move {
    int row;
    int col;
    int player;

    Move() : row(-1), col(-1), player(0) {}
    Move(int r, int c, int p) : row(r), col(c), player(p) {}
};

/**
 * @brief 游戏控制器
 *
 * 管理游戏逻辑、棋盘状态和胜负判断
 */
class GameController : public QObject
{
    Q_OBJECT

public:
    explicit GameController(QObject* parent = nullptr);
    ~GameController();

    // 棋子常量
    static const int EMPTY = 0;
    static const int BLACK = 1;
    static const int WHITE = 2;

    /**
     * @brief 开始游戏
     */
    void startGame();

    /**
     * @brief 落子
     * @param row 行号
     * @param col 列号
     * @param player 玩家
     * @return 是否成功
     */
    bool makeMove(int row, int col, int player);

    /**
     * @brief 悔棋
     * @return 是否成功
     */
    bool undoMove();

    /**
     * @brief 检查胜利
     * @param row 行号
     * @param col 列号
     * @param player 玩家
     * @return 是否胜利
     */
    int checkWin(int row, int col, int player);

    /**
     * @brief 获取游戏状态
     */
    GameState getGameState() const;

    /**
     * @brief 获取当前回合玩家
     */
    int getCurrentTurn() const;

    /**
     * @brief 获取棋步历史
     */
    QVector<Move> getMoveHistory() const;

    /**
     * @brief 重置游戏
     */
    void reset();

    /**
     * @brief 获取棋盘状态
     */
    void getBoard(int board[18][18]) const;

    /**
     * @brief 检查位置是否有效
     */
    bool isValidMove(int row, int col) const;

    /**
     * @brief 检查游戏是否结束
     */
    bool isGameOver() const;

    /**
     * @brief 获取获胜者
     */
    int getWinner() const;

signals:
    /**
     * @brief 游戏状态改变信号
     */
    void gameStateChanged(GameState state);

    /**
     * @brief 落子成功信号
     */
    void moveMade(const Move& move);

    /**
     * @brief 游戏结束信号
     */
    void gameOver(GameState state, int winner);

private:
    /**
     * @brief 检查五子连珠
     */
    bool checkFiveInRow(int row, int col, int player);

private:
    int board_[18][18];
    int currentTurn_;
    GameState gameState_;
    QVector<Move> moveHistory_;
    int winner_;
};

#endif // GAMECONTROLLER_H