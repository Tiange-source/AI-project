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
 * 提供三种难度的AI对手
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
     * @brief 中等难度：基于评分
     */
    AIMove computeMediumMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 困难难度：极大极小算法
     */
    AIMove computeHardMove(const int board[18][18], int aiPlayer);

    /**
     * @brief 评估位置分数
     */
    int evaluatePosition(const int board[18][18], int row, int col, int player);

    /**
     * @brief 获取有效落子位置
     */
    QVector<AIMove> getValidMoves(const int board[18][18]);

    /**
     * @brief 极大极小算法
     */
    int minimax(int board[18][18], int depth, bool maximizing, int alpha, int beta, int player);

    /**
     * @brief 评估棋盘
     */
    int evaluateBoard(const int board[18][18], int player);

    /**
     * @brief 检查五子连珠
     */
    int checkFiveInRow(const int board[18][18], int row, int col, int player);

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
    static const int SEARCH_DEPTH = 4;
};

#endif // AIENGINE_H