#include "gamecontroller.h"
#include <QDebug>

GameController::GameController(QObject* parent)
    : QObject(parent)
    , currentTurn_(BLACK)
    , gameState_(IDLE)
    , winner_(0)
{
    reset();
}

GameController::~GameController()
{
}

void GameController::startGame()
{
    reset();
    gameState_ = PLAYING;
    currentTurn_ = BLACK;
    emit gameStateChanged(gameState_);
    qDebug() << "Game started";
}

bool GameController::makeMove(int row, int col, int player)
{
    if (gameState_ != PLAYING) {
        qWarning() << "Game is not in playing state";
        return false;
    }

    if (!isValidMove(row, col)) {
        qWarning() << "Invalid move:" << row << col;
        return false;
    }

    if (player != currentTurn_) {
        qWarning() << "Not your turn";
        return false;
    }

    board_[row][col] = player;
    Move move(row, col, player);
    moveHistory_.append(move);

    emit moveMade(move);

    // 检查胜利
    int result = checkWin(row, col, player);
    if (result == player) {
        gameState_ = (player == BLACK) ? WIN : LOSE;
        winner_ = player;
        emit gameOver(gameState_, winner_);
        emit gameStateChanged(gameState_);
        qDebug() << "Game over, winner:" << winner_;
    } else {
        // 检查平局
        bool isFull = true;
        for (int i = 0; i < 18; ++i) {
            for (int j = 0; j < 18; ++j) {
                if (board_[i][j] == EMPTY) {
                    isFull = false;
                    break;
                }
            }
            if (!isFull) break;
        }

        if (isFull) {
            gameState_ = DRAW;
            emit gameOver(gameState_, 0);
            emit gameStateChanged(gameState_);
            qDebug() << "Game draw";
        } else {
            // 切换回合
            currentTurn_ = (currentTurn_ == BLACK) ? WHITE : BLACK;
        }
    }

    return true;
}

bool GameController::undoMove()
{
    if (moveHistory_.isEmpty()) {
        return false;
    }

    Move lastMove = moveHistory_.takeLast();
    board_[lastMove.row][lastMove.col] = EMPTY;
    currentTurn_ = lastMove.player;
    gameState_ = PLAYING;

    return true;
}

int GameController::checkWin(int row, int col, int player)
{
    if (checkFiveInRow(row, col, player)) {
        return player;
    }
    return 0;
}

GameState GameController::getGameState() const
{
    return gameState_;
}

int GameController::getCurrentTurn() const
{
    return currentTurn_;
}

QVector<Move> GameController::getMoveHistory() const
{
    return moveHistory_;
}

void GameController::reset()
{
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            board_[i][j] = EMPTY;
        }
    }
    currentTurn_ = BLACK;
    gameState_ = IDLE;
    moveHistory_.clear();
    winner_ = 0;
    qDebug() << "Game reset";
}

void GameController::getBoard(int board[18][18]) const
{
    for (int i = 0; i < 18; ++i) {
        for (int j = 0; j < 18; ++j) {
            board[i][j] = board_[i][j];
        }
    }
}

bool GameController::isValidMove(int row, int col) const
{
    return row >= 0 && row < 18 && col >= 0 && col < 18 && board_[row][col] == EMPTY;
}

bool GameController::isGameOver() const
{
    return gameState_ == WIN || gameState_ == LOSE || gameState_ == DRAW;
}

int GameController::getWinner() const
{
    return winner_;
}

bool GameController::checkFiveInRow(int row, int col, int player)
{
    // 检查四个方向：水平、垂直、主对角线、副对角线
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; ++d) {
        int count = 1;

        // 向一个方向延伸
        for (int i = 1; i < 5; ++i) {
            int newRow = row + i * directions[d][0];
            int newCol = col + i * directions[d][1];
            if (newRow >= 0 && newRow < 18 && newCol >= 0 && newCol < 18 &&
                board_[newRow][newCol] == player) {
                count++;
            } else {
                break;
            }
        }

        // 向相反方向延伸
        for (int i = 1; i < 5; ++i) {
            int newRow = row - i * directions[d][0];
            int newCol = col - i * directions[d][1];
            if (newRow >= 0 && newRow < 18 && newCol >= 0 && newCol < 18 &&
                board_[newRow][newCol] == player) {
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