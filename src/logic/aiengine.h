#ifndef AIENGINE_H
#define AIENGINE_H

#include <QObject>
#include <QVector>

/**
 * @brief 棋步结构
 */
struct AIMove {
    int row;
    int col;
    int score;

    AIMove() : row(-1), col(-1), score(0) {}
    AIMove(int r, int c, int s) : row(r), col(c), score(s) {}
};

/**
 * @brief AI引擎
 *
 * 使用五元组方案提供AI对战
 */
class AIEngine : public QObject
{
    Q_OBJECT

public:
    explicit AIEngine(QObject* parent = nullptr);
    ~AIEngine();

    // 难度级别
    static const int EASY = 1;
    static const int MEDIUM = 2;
    static const int HARD = 3;

    // 棋子常量
    static const int EMPTY = 0;
    static const int BLACK = 1;
    static const int WHITE = 2;

    /**
     * @brief 计算最佳落子位置
     * @param board 棋盘
     * @param aiPlayer AI玩家
     * @return 最佳落子位置
     */
    AIMove computeMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 设置难度
     * @param difficulty 难度级别
     */
    void setDifficulty(int difficulty);

    /**
     * @brief 获取难度
     */
    int getDifficulty() const;

private:
    /**
     * @brief 简单难度：随机落子
     */
    AIMove computeEasyMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 中等难度：五元组评分
     */
    AIMove computeMediumMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 困难难度：五元组评分 + 搜索
     */
    AIMove computeHardMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 五元组评分
     * @param board 棋盘
     * @param row 行号
     * @param col 列号
     * @param player 玩家
     * @return 分数
     */
    int evaluateFiveTuple(const int board[18][18], int row, int col, int player);

    /**
     * @brief 评估棋盘（五元组方案）
     * @param board 棋盘
     * @param player 玩家
     * @return 分数
     */
    int evaluateBoardFiveTuple(const int board[18][18], int player);

    /**
     * @brief 检查某个方向上的五元组
     * @param board 棋盘
     * @param row 起始行
     * @param col 起始列
     * @param dx 行方向
     * @param dy 列方向
     * @param player 玩家
     * @return 五元组类型和分数
     */
    QPair<int, int> checkLine(const int board[18][18], int row, int col, int dx, int dy, int player);

    /**
     * @brief 获取有效落子位置
     */
    QVector<AIMove> getValidMoves(const int board[18][18]);

    /**
     * @brief 极大极小算法（带五元组评估）
     */
    int minimax(int board[18][18], int depth, bool maximizing, int alpha, int beta, int player);

    /**
     * @brief 检查五子连珠
     */
    bool checkFiveInRow(const int board[18][18], int row, int col, int player);

    /**
     * @brief 检查位置是否有效
     */
    bool isValidPosition(int row, int col) const;

    /**
     * @brief 复制棋盘
     */
    void copyBoard(const int src[18][18], int dst[18][18]);

private:
    int difficulty_;
    static const int SEARCH_DEPTH = 3;

    // 五元组评分权重
    static const int SCORE_FIVE = 100000;        // 五连
    static const int SCORE_LIVE_FOUR = 10000;   // 活四
    static const int SCORE_DEAD_FOUR = 1000;    // 死四
    static const int SCORE_LIVE_THREE = 1000;   // 活三
    static const int SCORE_DEAD_THREE = 100;    // 死三
    static const int SCORE_LIVE_TWO = 100;      // 活二
    static const int SCORE_DEAD_TWO = 10;       // 死二
    static const int SCORE_ONE = 1;             // 单子
};

#endif // AIENGINE_H