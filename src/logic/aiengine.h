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
 * 使用五元组算法提供AI对战
 */
class AIEngine : public QObject
{
    Q_OBJECT

public:
    explicit AIEngine(QObject* parent = nullptr);
    ~AIEngine();

    // 棋子常量
    static const int EMPTY = 0;
    static const int BLACK = 1;  // 电脑
    static const int WHITE = 2;  // 玩家

    /**
     * @brief 计算最佳落子位置
     * @param board 棋盘
     * @param aiPlayer AI玩家（应该为BLACK）
     * @return 最佳落子位置
     */
    AIMove computeMove(const int board[18][18], int aiPlayer);

private:
    /**
     * @brief 获取包含(x,y)点四个方向的五元组情况
     * @param board 棋盘
     * @param x 行号
     * @param y 列号
     * @return 包含该点的五元组中黑棋和白棋的数量
     */
    QVector<int> getNums1(const int board[18][18], int x, int y);  // 水平方向
    QVector<int> getNums2(const int board[18][18], int x, int y);  // 垂直方向
    QVector<int> getNums3(const int board[18][18], int x, int y);  // 主对角线方向
    QVector<int> getNums4(const int board[18][18], int x, int y);  // 副对角线方向

    /**
     * @brief 一个五元组内，根据敌我棋的数量获取得分
     * @param nums 包含黑棋和白棋数量的数组 [黑棋数, 白棋数]
     * @param ch 评估对象（BLACK=电脑，WHITE=玩家）
     * @return 该五元组的得分
     */
    int xPoints(const QVector<int>& nums, int ch);

    /**
     * @brief 获取该点的得分
     * @param board 棋盘
     * @param x 行号
     * @param y 列号
     * @param ch 评估对象（BLACK=电脑，WHITE=玩家）
     * @return 该点的总得分
     */
    int getPoints(const int board[18][18], int x, int y, int ch);

    /**
     * @brief 检查位置是否有效
     */
    bool isValidPosition(int x, int y) const;

    /**
     * @brief 获取有效落子位置（周围有棋子的空位）
     */
    QVector<AIMove> getValidMoves(const int board[18][18]);

    /**
     * @brief 检查五子连珠
     */
    bool checkFiveInRow(const int board[18][18], int x, int y, int player);
};

#endif // AIENGINE_H