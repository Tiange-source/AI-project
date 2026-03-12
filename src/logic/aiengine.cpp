#include "aiengine.h"
#include <QDebug>
#include <QRandomGenerator>
#include <algorithm>
#include <ctime>

AIEngine::AIEngine(QObject* parent)
    : QObject(parent)
{
    // 初始化随机数种子
    qsrand(static_cast<uint>(time(nullptr)));
}

AIEngine::~AIEngine()
{
}

AIMove AIEngine::computeMove(const int board[18][18], int aiPlayer)
{
    // 如果是第一步，下在中心附近
    int steps = 0;
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] != EMPTY) {
                steps++;
            }
        }
    }

    if (steps == 0) {
        // 第一步，下在中心
        return AIMove(9, 9, 0);
    }

    QVector<AIMove> validMoves = getValidMoves(board, steps);

    if (validMoves.isEmpty()) {
        // 如果没有有效位置，返回中心位置
        return AIMove(9, 9, 0);
    }

    // 评估每个位置的得分
    AIMove bestMove = validMoves[0];
    int maxPoints = -1;

    // 使用随机数种子
    QRandomGenerator* rng = QRandomGenerator::global();

    // 打乱顺序，避免每次都选择相同的位置
    std::shuffle(validMoves.begin(), validMoves.end(), *rng);

    for (auto& move : validMoves) {
        // 计算该位置的得分（进攻得分+防守得分）
        int attackPoints = getPoints(board, move.row, move.col, BLACK);
        int defensePoints = getPoints(board, move.row, move.col, WHITE);

        // 综合得分，优先选择得分最高的位置
        int totalPoints = attackPoints + defensePoints;
        move.score = totalPoints;

        if (totalPoints > maxPoints) {
            maxPoints = totalPoints;
            bestMove = move;
        }
    }

    return bestMove;
}

QVector<int> AIEngine::getNums1(const int board[18][18], int x, int y)
{
    // 水平方向：向左延伸5格
    int k = 0;  // 电脑棋子数（黑棋）
    int t = 0;  // 玩家棋子数（白棋）
    int cnt = 5;

    while (cnt-- && isOk(x, y)) {
        if (board[x][y] == BLACK) k++;
        if (board[x][y] == WHITE) t++;
        y--;
    }

    if (cnt >= 0) {
        return QVector<int>();  // 超出边界，返回空
    }

    return QVector<int>{k, t};
}

QVector<int> AIEngine::getNums2(const int board[18][18], int x, int y)
{
    // 垂直方向：向上延伸5格
    int k = 0;  // 电脑棋子数（黑棋）
    int t = 0;  // 玩家棋子数（白棋）
    int cnt = 5;

    while (cnt-- && isOk(x, y)) {
        if (board[x][y] == BLACK) k++;
        if (board[x][y] == WHITE) t++;
        x--;
    }

    if (cnt >= 0) {
        return QVector<int>();  // 超出边界，返回空
    }

    return QVector<int>{k, t};
}

QVector<int> AIEngine::getNums3(const int board[18][18], int x, int y)
{
    // 主对角线方向（右下）：向左上延伸5格
    int k = 0;  // 电脑棋子数（黑棋）
    int t = 0;  // 玩家棋子数（白棋）
    int cnt = 5;

    while (cnt-- && isOk(x, y)) {
        if (board[x][y] == BLACK) k++;
        if (board[x][y] == WHITE) t++;
        x++;
        y--;
    }

    if (cnt >= 0) {
        return QVector<int>();  // 超出边界，返回空
    }

    return QVector<int>{k, t};
}

QVector<int> AIEngine::getNums4(const int board[18][18], int x, int y)
{
    // 副对角线方向（左下）：向右上延伸5格
    int k = 0;  // 电脑棋子数（黑棋）
    int t = 0;  // 玩家棋子数（白棋）
    int cnt = 5;

    while (cnt-- && isOk(x, y)) {
        if (board[x][y] == BLACK) k++;
        if (board[x][y] == WHITE) t++;
        x--;
        y--;
    }

    if (cnt >= 0) {
        return QVector<int>();  // 超出边界，返回空
    }

    return QVector<int>{k, t};
}

int AIEngine::xPoints(const QVector<int>& nums, int ch)
{
    if (nums.isEmpty()) {
        return 0;
    }

    int g, b;
    if (ch == BLACK) {
        // ch = 1，获取己方得分
        g = nums[0];  // 黑棋数
        b = nums[1];  // 白棋数
    } else {
        // ch = 0，获取敌方得分
        g = nums[1];  // 白棋数
        b = nums[0];  // 黑棋数
    }

    // 评分规则
    if (g == 0 && b == 0) {
        return 10;
    } else if (g == 1 && b == 0) {
        return 35;
    } else if (g == 2 && b == 0) {
        return 1500;
    } else if (g == 3 && b == 0) {
        return 18000;
    } else if (g == 4 && b == 0) {
        return 1000000;
    } else if (g == 0 && b == 1) {
        return 15;
    } else if (g == 0 && b == 2) {
        return 400;
    } else if (g == 0 && b == 3) {
        return 6000;
    } else if (g == 0 && b == 4) {
        return 150000;
    } else if (g != 0 && b != 0) {
        return 0;  // 既有黑棋又有白棋，该五元组无效
    }

    return 0;
}

int AIEngine::getPoints(const int board[18][18], int x, int y, int ch)
{
    int ret = 0;
    int x1 = x, y1 = y;
    int x2 = x, y2 = y;
    int x3 = x, y3 = y;
    int x4 = x, y4 = y;

    // 检查四个方向的五元组
    for (int i = 0; i < 5; i++) {
        // 水平方向
        if (isOk(x1, y1)) {
            ret += xPoints(getNums1(board, x1, y1), ch);
            y1++;
        }

        // 垂直方向
        if (isOk(x2, y2)) {
            ret += xPoints(getNums2(board, x2, y2), ch);
            x2++;
        }

        // 主对角线方向
        if (isOk(x3, y3)) {
            ret += xPoints(getNums3(board, x3, y3), ch);
            x3--;
            y3++;
        }

        // 副对角线方向
        if (isOk(x4, y4)) {
            ret += xPoints(getNums4(board, x4, y4), ch);
            x4++;
            y4++;
        }
    }

    return ret;
}

double AIEngine::getNowPoints(const int board[18][18], int ch)
{
    double ret = 0;

    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] == ch) {
                ret += getPoints(board, i, j, ch) / 100.0;
            }
        }
    }

    return ret;
}

bool AIEngine::isOk(int x, int y) const
{
    return x >= 0 && x < 18 && y >= 0 && y < 18;
}

QVector<AIMove> AIEngine::getValidMoves(const int board[18][18], int steps)
{
    QVector<AIMove> moves;

    // 检查是否有棋子
    if (steps == 0) {
        moves.append(AIMove(9, 9, 0));
        return moves;
    }

    // 根据步数调整搜索范围
    // 步数越少，搜索范围越大
    int searchRange = 2;
    if (steps < 10) {
        searchRange = 3;
    } else if (steps < 30) {
        searchRange = 2;
    } else {
        searchRange = 1;
    }

    // 评估每个空位，只保留有价值的落子点
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] == EMPTY) {
                // 检查周围是否有棋子
                bool hasNeighbor = false;
                for (int di = -searchRange; di <= searchRange; ++di) {
                    for (int dj = -searchRange; dj <= searchRange; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di;
                        int nj = j + dj;
                        if (isOk(ni, nj) && board[ni][nj] != EMPTY) {
                            hasNeighbor = true;
                            break;
                        }
                    }
                    if (hasNeighbor) break;
                }

                // 只考虑周围有棋子的位置
                if (hasNeighbor) {
                    moves.append(AIMove(i, j, 0));
                }
            }
        }
    }

    // 如果没有周围有棋子的位置，返回所有空位（用于开局）
    if (moves.isEmpty()) {
        for (int i = 0; i < 18; ++i) {
            for (int j = 0; j < 18; ++j) {
                if (board[i][j] == EMPTY) {
                    moves.append(AIMove(i, j, 0));
                }
            }
        }
    }

    return moves;
}

bool AIEngine::checkFiveInRow(const int board[18][18], int x, int y, int player)
{
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; ++d) {
        int count = 1;

        for (int i = 1; i < 5; ++i) {
            int newX = x + i * directions[d][0];
            int newY = y + i * directions[d][1];
            if (isOk(newX, newY) && board[newX][newY] == player) {
                count++;
            } else {
                break;
            }
        }

        for (int i = 1; i < 5; ++i) {
            int newX = x - i * directions[d][0];
            int newY = y - i * directions[d][1];
            if (isOk(newX, newY) && board[newX][newY] == player) {
                count++;
            } else {
                break;
            }
        }

        if (count >= 5) {
            return true;
        }
    }

    return false;
}