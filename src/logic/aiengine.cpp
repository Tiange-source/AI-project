#include "aiengine.h"
#include <QDebug>
#include <QRandomGenerator>
#include <algorithm>

AIEngine::AIEngine(QObject* parent)
    : QObject(parent)
{
}

AIEngine::~AIEngine()
{
}

AIMove AIEngine::computeMove(const int board[18][18], int aiPlayer)
{
    QVector<AIMove> validMoves = getValidMoves(board);

    if (validMoves.isEmpty()) {
        // 如果没有有效位置，返回中心位置
        return AIMove(9, 9, 0);
    }

    // 评估每个位置的得分
    AIMove bestMove = validMoves[0];
    int maxScore = -1;

    for (auto& move : validMoves) {
        // 计算该位置的得分（进攻得分+防守得分）
        int attackScore = getPoints(board, move.row, move.col, BLACK);
        int defenseScore = getPoints(board, move.row, move.col, WHITE);

        // 综合得分，优先选择得分最高的位置
        int totalScore = attackScore + defenseScore;
        move.score = totalScore;

        if (totalScore > maxScore) {
            maxScore = totalScore;
            bestMove = move;
        }
    }

    return bestMove;
}

QVector<int> AIEngine::getNums1(const int board[18][18], int x, int y)
{
    // 水平方向：从该点开始，左右各延伸最多4格，形成一个五元组
    int g = 0;  // 电脑棋子数（黑棋）
    int b = 0;  // 玩家棋子数（白棋）

    // 向右延伸
    for (int i = 0; i < 5 && isValidPosition(x, y + i); ++i) {
        if (board[x][y + i] == BLACK) {
            g++;
        } else if (board[x][y + i] == WHITE) {
            b++;
        }
    }

    // 如果总棋子数超过5，需要从该点向左调整起点
    int offset = 0;
    if (g + b > 5) {
        // 尝试向左移动起点
        for (offset = 0; offset <= 4 && isValidPosition(x, y - offset); ++offset) {
            int newG = 0;
            int newB = 0;
            for (int i = 0; i < 5 && isValidPosition(x, y - offset + i); ++i) {
                if (board[x][y - offset + i] == BLACK) {
                    newG++;
                } else if (board[x][y - offset + i] == WHITE) {
                    newB++;
                }
            }
            if (newG + newB <= 5) {
                g = newG;
                b = newB;
                break;
            }
        }
    }

    return QVector<int>{g, b};
}

QVector<int> AIEngine::getNums2(const int board[18][18], int x, int y)
{
    // 垂直方向
    int g = 0;  // 电脑棋子数（黑棋）
    int b = 0;  // 玩家棋子数（白棋）

    // 向下延伸
    for (int i = 0; i < 5 && isValidPosition(x + i, y); ++i) {
        if (board[x + i][y] == BLACK) {
            g++;
        } else if (board[x + i][y] == WHITE) {
            b++;
        }
    }

    // 如果总棋子数超过5，需要从该点向上调整起点
    int offset = 0;
    if (g + b > 5) {
        for (offset = 0; offset <= 4 && isValidPosition(x - offset, y); ++offset) {
            int newG = 0;
            int newB = 0;
            for (int i = 0; i < 5 && isValidPosition(x - offset + i, y); ++i) {
                if (board[x - offset + i][y] == BLACK) {
                    newG++;
                } else if (board[x - offset + i][y] == WHITE) {
                    newB++;
                }
            }
            if (newG + newB <= 5) {
                g = newG;
                b = newB;
                break;
            }
        }
    }

    return QVector<int>{g, b};
}

QVector<int> AIEngine::getNums3(const int board[18][18], int x, int y)
{
    // 主对角线方向（左上到右下）
    int g = 0;  // 电脑棋子数（黑棋）
    int b = 0;  // 玩家棋子数（白棋）

    // 向右下延伸
    for (int i = 0; i < 5 && isValidPosition(x + i, y + i); ++i) {
        if (board[x + i][y + i] == BLACK) {
            g++;
        } else if (board[x + i][y + i] == WHITE) {
            b++;
        }
    }

    // 如果总棋子数超过5，需要从该点向左上调整起点
    int offset = 0;
    if (g + b > 5) {
        for (offset = 0; offset <= 4 && isValidPosition(x - offset, y - offset); ++offset) {
            int newG = 0;
            int newB = 0;
            for (int i = 0; i < 5 && isValidPosition(x - offset + i, y - offset + i); ++i) {
                if (board[x - offset + i][y - offset + i] == BLACK) {
                    newG++;
                } else if (board[x - offset + i][y - offset + i] == WHITE) {
                    newB++;
                }
            }
            if (newG + newB <= 5) {
                g = newG;
                b = newB;
                break;
            }
        }
    }

    return QVector<int>{g, b};
}

QVector<int> AIEngine::getNums4(const int board[18][18], int x, int y)
{
    // 副对角线方向（右上到左下）
    int g = 0;  // 电脑棋子数（黑棋）
    int b = 0;  // 玩家棋子数（白棋）

    // 向左下延伸
    for (int i = 0; i < 5 && isValidPosition(x + i, y - i); ++i) {
        if (board[x + i][y - i] == BLACK) {
            g++;
        } else if (board[x + i][y - i] == WHITE) {
            b++;
        }
    }

    // 如果总棋子数超过5，需要从该点向右上调整起点
    int offset = 0;
    if (g + b > 5) {
        for (offset = 0; offset <= 4 && isValidPosition(x - offset, y + offset); ++offset) {
            int newG = 0;
            int newB = 0;
            for (int i = 0; i < 5 && isValidPosition(x - offset + i, y + offset - i); ++i) {
                if (board[x - offset + i][y + offset - i] == BLACK) {
                    newG++;
                } else if (board[x - offset + i][y + offset - i] == WHITE) {
                    newB++;
                }
            }
            if (newG + newB <= 5) {
                g = newG;
                b = newB;
                break;
            }
        }
    }

    return QVector<int>{g, b};
}

int AIEngine::xPoints(const QVector<int>& nums, int ch)
{
    int g = nums[0];  // 电脑棋子数（黑棋）
    int b = nums[1];  // 玩家棋子数（白棋）

    // 根据用户提供的评分规则
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
    // 获取四个方向的五元组情况
    QVector<int> nums1 = getNums1(board, x, y);
    QVector<int> nums2 = getNums2(board, x, y);
    QVector<int> nums3 = getNums3(board, x, y);
    QVector<int> nums4 = getNums4(board, x, y);

    // 计算每个方向的得分
    int score1 = xPoints(nums1, ch);
    int score2 = xPoints(nums2, ch);
    int score3 = xPoints(nums3, ch);
    int score4 = xPoints(nums4, ch);

    // 累加所有方向的得分
    return score1 + score2 + score3 + score4;
}

bool AIEngine::isValidPosition(int x, int y) const
{
    return x >= 0 && x < 18 && y >= 0 && y < 18;
}

QVector<AIMove> AIEngine::getValidMoves(const int board[18][18])
{
    QVector<AIMove> moves;
    bool hasMoves = false;

    // 首先检查是否有棋子
    for (int i = 0; i < 18 && !hasMoves; ++i) {
        for (int j = 0; j < 18 && !hasMoves; ++j) {
            if (board[i][j] != EMPTY) {
                hasMoves = true;
                break;
            }
        }
    }

    if (!hasMoves) {
        // 棋盘为空，返回中心位置
        moves.append(AIMove(9, 9, 0));
        return moves;
    }

    // 评估每个空位，只保留有价值的落子点
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] == EMPTY) {
                // 检查周围是否有棋子（2格范围内）
                bool hasNeighbor = false;
                for (int di = -2; di <= 2; ++di) {
                    for (int dj = -2; dj <= 2; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di;
                        int nj = j + dj;
                        if (isValidPosition(ni, nj) && board[ni][nj] != EMPTY) {
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
            if (isValidPosition(newX, newY) && board[newX][newY] == player) {
                count++;
            } else {
                break;
            }
        }

        for (int i = 1; i < 5; ++i) {
            int newX = x - i * directions[d][0];
            int newY = y - i * directions[d][1];
            if (isValidPosition(newX, newY) && board[newX][newY] == player) {
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