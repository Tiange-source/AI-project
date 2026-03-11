#include "business/GameController.h"
#include "utils/Logger.h"

namespace gomoku {

GameController::GameController() {
    board_.resize(BOARD_SIZE, std::vector<int>(BOARD_SIZE, 0));
    initGame();
}

GameController::~GameController() {
}

void GameController::initGame() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 清空棋盘
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            board_[i][j] = 0;
        }
    }
    
    currentPlayer_ = 1;  // 玩家1先手
    moveHistory_.clear();
    gameOver_ = false;
    winner_ = 0;
    
    LOG_INFO("GameController::initGame - game initialized");
}

bool GameController::makeMove(int row, int col, int playerId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查游戏是否已结束
    if (gameOver_) {
        LOG_WARN("GameController::makeMove - game is over");
        return false;
    }
    
    // 检查是否轮到该玩家
    if (playerId != currentPlayer_) {
        LOG_WARN("GameController::makeMove - not player's turn");
        return false;
    }
    
    // 检查落子是否合法
    if (!isValidMove(row, col)) {
        LOG_WARN("GameController::makeMove - invalid move at (" + std::to_string(row) + ", " + std::to_string(col) + ")");
        return false;
    }
    
    // 落子
    board_[row][col] = playerId;
    
    // 记录落子
    Move move;
    move.row = row;
    move.col = col;
    move.playerId = playerId;
    moveHistory_.push_back(move);
    
    // 检查胜负
    GameResult result = checkWin(row, col, playerId);
    if (result == GameResult::WIN) {
        gameOver_ = true;
        winner_ = playerId;
        LOG_INFO("GameController::makeMove - player " + std::to_string(playerId) + " wins!");
    } else if (moveHistory_.size() == BOARD_SIZE * BOARD_SIZE) {
        // 棋盘已满，平局
        gameOver_ = true;
        winner_ = 0;
        LOG_INFO("GameController::makeMove - game draw");
    } else {
        // 切换玩家
        switchPlayer();
    }
    
    return true;
}

bool GameController::isValidMove(int row, int col) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检查位置是否在棋盘内
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return false;
    }
    
    // 检查位置是否为空
    return board_[row][col] == 0;
}

GameResult GameController::checkWin(int row, int col, int playerId) const {
    // 检查四个方向：水平、垂直、主对角线、副对角线
    if (checkDirection(row, col, playerId, 0, 1)) {  // 水平
        return GameResult::WIN;
    }
    if (checkDirection(row, col, playerId, 1, 0)) {  // 垂直
        return GameResult::WIN;
    }
    if (checkDirection(row, col, playerId, 1, 1)) {  // 主对角线
        return GameResult::WIN;
    }
    if (checkDirection(row, col, playerId, 1, -1)) { // 副对角线
        return GameResult::WIN;
    }
    
    return GameResult::CONTINUE;
}

const std::vector<std::vector<int>>& GameController::getBoard() const {
    return board_;
}

int GameController::getCurrentPlayer() const {
    return currentPlayer_;
}

void GameController::switchPlayer() {
    currentPlayer_ = (currentPlayer_ == 1) ? 2 : 1;
}

const std::vector<Move>& GameController::getMoveHistory() const {
    return moveHistory_;
}

bool GameController::undoMove() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (moveHistory_.empty()) {
        return false;
    }
    
    // 撤销最后一步
    Move lastMove = moveHistory_.back();
    board_[lastMove.row][lastMove.col] = 0;
    moveHistory_.pop_back();
    
    // 切换回上一个玩家
    currentPlayer_ = lastMove.playerId;
    
    // 如果游戏已结束，重置游戏状态
    if (gameOver_) {
        gameOver_ = false;
        winner_ = 0;
    }
    
    LOG_INFO("GameController::undoMove - move undone at (" + std::to_string(lastMove.row) + ", " + std::to_string(lastMove.col) + ")");
    return true;
}

void GameController::resetGame() {
    initGame();
}

bool GameController::isGameOver() const {
    return gameOver_;
}

int GameController::getWinner() const {
    return winner_;
}

bool GameController::checkDirection(int row, int col, int playerId, int dRow, int dCol) const {
    // 统计正反两个方向的连续棋子数
    int count = 1;  // 当前落子算1个
    
    // 正方向
    count += countConsecutive(row, col, playerId, dRow, dCol);
    
    // 反方向
    count += countConsecutive(row, col, playerId, -dRow, -dCol);
    
    return count >= WIN_COUNT;
}

int GameController::countConsecutive(int row, int col, int playerId, int dRow, int dCol) const {
    int count = 0;
    int r = row + dRow;
    int c = col + dCol;
    
    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && board_[r][c] == playerId) {
        count++;
        r += dRow;
        c += dCol;
    }
    
    return count;
}

} // namespace gomoku