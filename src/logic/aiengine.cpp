#include "aiengine.h"
#include <QDebug>
#include <QRandomGenerator>
#include <algorithm>

AIEngine::AIEngine(QObject* parent)
    : QObject(parent)
    , difficulty_(MEDIUM)
{
}

AIEngine::~AIEngine()
{
}

AIMove AIEngine::computeMove(const int board[18][18], int aiPlayer)
{
    switch (difficulty_) {
        case EASY:
            return computeEasyMove(board, aiPlayer);
        case MEDIUM:
            return computeMediumMove(board, aiPlayer);
        case HARD:
            return computeHardMove(board, aiPlayer);
        default:
            return computeMediumMove(board, aiPlayer);
    }
}

void AIEngine::setDifficulty(int difficulty)
{
    difficulty_ = difficulty;
}

int AIEngine::getDifficulty() const
{
    return difficulty_;
}

AIMove AIEngine::computeEasyMove(const int board[18][18], int aiPlayer)
{
    QVector<AIMove> validMoves = getValidMoves(board);

    if (validMoves.isEmpty()) {
        return AIMove();
    }

    // 随机选择一个位置
    int index = QRandomGenerator::global()->bounded(validMoves.size());
    return validMoves[index];
}

AIMove AIEngine::computeMediumMove(const int board[18][18], int aiPlayer)
{
    QVector<AIMove> validMoves = getValidMoves(board);

    if (validMoves.isEmpty()) {
        return AIMove();
    }

    // 评估每个位置的分数
    for (auto& move : validMoves) {
        move.score = evaluatePosition(board, move.row, move.col, aiPlayer);
    }

    // 选择分数最高的位置
    std::sort(validMoves.begin(), validMoves.end(),
              [](const AIMove& a, const AIMove& b) {
                  return a.score > b.score;
              });

    return validMoves[0];
}

AIMove AIEngine::computeHardMove(const int board[18][18], int aiPlayer)
{
    QVector<AIMove> validMoves = getValidMoves(board);

    if (validMoves.isEmpty()) {
        return AIMove();
    }

    // 评估每个位置的分数
    for (auto& move : validMoves) {
        int tempBoard[18][18];
        copyBoard(board, tempBoard);
        tempBoard[move.row][move.col] = aiPlayer;
        move.score = evaluateBoard(tempBoard, aiPlayer);
    }

    // 选择分数最高的位置
    std::sort(validMoves.begin(), validMoves.end(),
              [](const AIMove& a, const AIMove& b) {
                  return a.score > b.score;
              });

    return validMoves[0];
}

int AIEngine::evaluatePosition(const int board[18][18], int row, int col, int player)
{
    int score = 0;

    // 检查四个方向
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; ++d) {
        int count = 1;
        int blocked = 0;

        // 向一个方向延伸
        for (int i = 1; i < 5; ++i) {
            int newRow = row + i * directions[d][0];
            int newCol = col + i * directions[d][1];
            if (!isValidPosition(newRow, newCol)) {
                blocked++;
                break;
            }
            if (board[newRow][newCol] == player) {
                count++;
            } else if (board[newRow][newCol] == EMPTY) {
                break;
            } else {
                blocked++;
                break;
            }
        }

        // 向相反方向延伸
        for (int i = 1; i < 5; ++i) {
            int newRow = row - i * directions[d][0];
            int newCol = col - i * directions[d][1];
            if (!isValidPosition(newRow, newCol)) {
                blocked++;
                break;
            }
            if (board[newRow][newCol] == player) {
                count++;
            } else if (board[newRow][newCol] == EMPTY) {
                break;
            } else {
                blocked++;
                break;
            }
        }

        // 根据连珠数和被阻挡情况评分
        if (blocked == 0) {
            if (count >= 5) score += 100000;
            else if (count == 4) score += 10000;
            else if (count == 3) score += 1000;
            else if (count == 2) score += 100;
        } else if (blocked == 1) {
            if (count >= 5) score += 10000;
            else if (count == 4) score += 1000;
            else if (count == 3) score += 100;
        }
    }

    return score;
}

QVector<AIMove> AIEngine::getValidMoves(const int board[18][18])
{
    QVector<AIMove> moves;

    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] == EMPTY) {
                // 检查周围是否有棋子
                bool hasNeighbor = false;
                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
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
                if (hasNeighbor) {
                    moves.append(AIMove(i, j, 0));
                }
            }
        }
    }

    // 如果没有周围有棋子的位置，返回中心位置
    if (moves.isEmpty()) {
        moves.append(AIMove(9, 9, 0));
    }

    return moves;
}

int AIEngine::minimax(int board[18][18], int depth, bool maximizing, int alpha, int beta, int player)
{
    // 简化的极大极小算法
    int opponent = (player == BLACK) ? WHITE : BLACK;

    // 检查胜利
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] != EMPTY) {
                if (checkFiveInRow(board, i, j, board[i][j])) {
                    if (board[i][j] == player) {
                        return 100000 - depth;
                    } else {
                        return -100000 + depth;
                    }
                }
            }
        }
    }

    if (depth == 0) {
        return evaluateBoard(board, player);
    }

    QVector<AIMove> validMoves = getValidMoves(board);
    if (validMoves.isEmpty()) {
        return 0;
    }

    if (maximizing) {
        int maxEval = -999999;
        for (const auto& move : validMoves) {
            int newBoard[18][18];
            copyBoard(board, newBoard);
            newBoard[move.row][move.col] = player;
            int eval = minimax(newBoard, depth - 1, false, alpha, beta, opponent);
            maxEval = qMax(maxEval, eval);
            alpha = qMax(alpha, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return maxEval;
    } else {
        int minEval = 999999;
        for (const auto& move : validMoves) {
            int newBoard[18][18];
            copyBoard(board, newBoard);
            newBoard[move.row][move.col] = player;
            int eval = minimax(newBoard, depth - 1, true, alpha, beta, opponent);
            minEval = qMin(minEval, eval);
            beta = qMin(beta, eval);
            if (beta <= alpha) {
                break;
            }
        }
        return minEval;
    }
}

int AIEngine::evaluateBoard(const int board[18][18], int player)
{
    int score = 0;
    int opponent = (player == BLACK) ? WHITE : BLACK;

    // 评估每个位置的分数
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            if (board[i][j] == player) {
                score += evaluatePosition(board, i, j, player);
            } else if (board[i][j] == opponent) {
                score -= evaluatePosition(board, i, j, opponent);
            }
        }
    }

    return score;
}

int AIEngine::checkFiveInRow(const int board[18][18], int row, int col, int player)
{
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; ++d) {
        int count = 1;

        for (int i = 1; i < 5; ++i) {
            int newRow = row + i * directions[d][0];
            int newCol = col + i * directions[d][1];
            if (isValidPosition(newRow, newCol) && board[newRow][newCol] == player) {
                count++;
            } else {
                break;
            }
        }

        for (int i = 1; i < 5; ++i) {
            int newRow = row - i * directions[d][0];
            int newCol = col - i * directions[d][1];
            if (isValidPosition(newRow, newCol) && board[newRow][newCol] == player) {
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

bool AIEngine::isValidPosition(int row, int col) const
{
    return row >= 0 && row < 18 && col >= 0 && col < 18;
}

void AIEngine::copyBoard(const int src[18][18], int dst[18][18])
{
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            dst[i][j] = src[i][j];
        }
    }
}