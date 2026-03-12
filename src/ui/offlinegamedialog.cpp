#include "offlinegamedialog.h"
#include "ui_offlinegamedialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QGraphicsDropShadowEffect>

OfflineGameDialog::OfflineGameDialog(QWidget* parent)
    : QDialog(parent)
    , ui_(new Ui::OfflineGameDialog)
    , boardWidget_(new BoardWidget(this))
    , gameController_(new GameController(this))
    , aiEngine_(new AIEngine(this))
    , userProfile_(UserProfile::create())
    , turnLabel_(nullptr)
    , undoButton_(nullptr)
    , restartButton_(nullptr)
    , backButton_(nullptr)
    , playerColor_(1)  // 默认玩家使用黑棋
    , isAITurn_(false)
{
    ui_->setupUi(this);
    initUI();
    connectSignals();

    // 设置无边框对话框
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

    // 显示颜色选择对话框
    showColorSelectionDialog();
}

OfflineGameDialog::~OfflineGameDialog()
{
    delete ui_;
}

void OfflineGameDialog::initUI()
{
    // 设置对话框大小
    setFixedSize(900, 700);

    // 设置阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 150));
    shadow->setOffset(0, 5);
    setGraphicsEffect(shadow);

    // 创建主容器
    QWidget* mainContainer = new QWidget(this);
    mainContainer->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #667eea, stop:1 #764ba2);
            border-radius: 20px;
        }
    )");

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 标题栏
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("离线对战", mainContainer);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: #ffffff;
            font-size: 24px;
            font-weight: bold;
            background: transparent;
        }
    )");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    mainLayout->addLayout(titleLayout);

    // 游戏信息
    QHBoxLayout* infoLayout = new QHBoxLayout();
    turnLabel_ = new QLabel("当前回合: 白棋（玩家）", mainContainer);
    turnLabel_->setStyleSheet(R"(
        QLabel {
            color: #ffffff;
            font-size: 16px;
            font-weight: bold;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 10px;
            padding: 8px 15px;
        }
    )");
    infoLayout->addWidget(turnLabel_);
    infoLayout->addStretch();
    mainLayout->addLayout(infoLayout);

    // 棋盘
    QVBoxLayout* boardLayout = new QVBoxLayout();
    boardLayout->addWidget(boardWidget_);
    mainLayout->addLayout(boardLayout);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);

    undoButton_ = new QPushButton("悔棋", mainContainer);
    undoButton_->setMinimumHeight(40);
    undoButton_->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 0.9);
            color: #667eea;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: #ffffff;
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.8);
        }
        QPushButton:disabled {
            background: rgba(255, 255, 255, 0.5);
            color: rgba(102, 126, 234, 0.5);
        }
    )");
    buttonLayout->addWidget(undoButton_);

    restartButton_ = new QPushButton("重新开始", mainContainer);
    restartButton_->setMinimumHeight(40);
    restartButton_->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #667eea, stop:1 #764ba2);
            color: #ffffff;
            border: none;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #5568d3, stop:1 #6a42a0);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #6a42a0, stop:1 #5568d3);
        }
    )");
    buttonLayout->addWidget(restartButton_);

    backButton_ = new QPushButton("返回", mainContainer);
    backButton_->setMinimumHeight(40);
    backButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ffffff;
            border: 2px solid #ffffff;
            border-radius: 10px;
            font-size: 14px;
            font-weight: 600;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.2);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.3);
        }
    )");
    buttonLayout->addWidget(backButton_);

    mainLayout->addLayout(buttonLayout);

    // 设置主布局
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->addWidget(mainContainer);

    setLayout(dialogLayout);
}

void OfflineGameDialog::connectSignals()
{
    connect(boardWidget_, &BoardWidget::pieceClicked, this, &OfflineGameDialog::onPieceClicked);
    connect(restartButton_, &QPushButton::clicked, this, &OfflineGameDialog::onRestartButtonClicked);
    connect(backButton_, &QPushButton::clicked, this, &OfflineGameDialog::onBackButtonClicked);
    connect(undoButton_, &QPushButton::clicked, this, &OfflineGameDialog::onUndoButtonClicked);
}

void OfflineGameDialog::onPieceClicked(int row, int col)
{
    if (isAITurn_) {
        return;
    }

    if (!gameController_->makeMove(row, col, playerColor_)) {
        return;
    }

    boardWidget_->setPiece(row, col, playerColor_);

    int winner = gameController_->checkWin(row, col, playerColor_);
    if (winner != 0) {
        endGame(winner);
        return;
    }

    isAITurn_ = true;
    updateGameInfo();

    // AI落子（延迟以显示玩家落子）
    QTimer::singleShot(500, this, [this]() {
        makeAIMove();
    });
}

void OfflineGameDialog::makeAIMove()
{
    if (!isAITurn_) {
        return;
    }

    int aiColor = (playerColor_ == 1) ? 2 : 1;
    int board[18][18];
    gameController_->getBoard(board);
    AIMove move = aiEngine_->computeMove(board, aiColor);

    if (move.row >= 0 && move.col >= 0) {
        gameController_->makeMove(move.row, move.col, aiColor);
        boardWidget_->setPiece(move.row, move.col, aiColor);

        int winner = gameController_->checkWin(move.row, move.col, aiColor);
        if (winner != 0) {
            endGame(winner);
            return;
        }
    }

    isAITurn_ = false;
    updateGameInfo();
}

void OfflineGameDialog::onRestartButtonClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "重新开始",
        "确定要重新开始游戏吗？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        gameController_->reset();
        gameController_->startGame();
        boardWidget_->reset();
        boardWidget_->setEnabled(true);
        isAITurn_ = false;
        updateGameInfo();
    }
}

void OfflineGameDialog::onBackButtonClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "返回",
        "确定要退出游戏吗？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        reject();
    }
}

void OfflineGameDialog::onUndoButtonClicked()
{
    if (isAITurn_) {
        return;
    }

    // 悔棋需要回退两步（玩家和AI各一步）
    if (gameController_->undoMove() && gameController_->undoMove()) {
        // 更新棋盘显示
        boardWidget_->reset();
        int board[18][18];
        gameController_->getBoard(board);
        for (int i = 0; i < 18; ++i) {
            for (int j = 0; j < 18; ++j) {
                if (board[i][j] != 0) {
                    boardWidget_->setPiece(i, j, board[i][j]);
                }
            }
        }
        updateGameInfo();
    }
}

void OfflineGameDialog::updateGameInfo()
{
    if (isAITurn_) {
        QString aiColorText = (playerColor_ == 1) ? "白棋" : "黑棋";
        turnLabel_->setText("当前回合: " + aiColorText + "（AI）");
    } else {
        QString playerColorText = (playerColor_ == 1) ? "黑棋" : "白棋";
        turnLabel_->setText("当前回合: " + playerColorText + "（玩家）");
    }

    // 更新悔棋按钮状态
    QVector<Move> moveHistory = gameController_->getMoveHistory();
    undoButton_->setEnabled(!isAITurn_ && moveHistory.size() >= 2);
}

void OfflineGameDialog::endGame(int winner)
{
    boardWidget_->setEnabled(false);
    isAITurn_ = false;

    QString message;
    if (winner == playerColor_) {
        message = "恭喜！你赢了！";
    } else {
        message = "很遗憾，AI赢了！";
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "游戏结束",
        message + "\n\n是否再来一局？",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        gameController_->reset();
        startGame();
    } else {
        accept();
    }
}

void OfflineGameDialog::showColorSelectionDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("选择棋子颜色");
    dialog.setFixedSize(400, 250);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground);

    // 设置阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(&dialog);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 150));
    shadow->setOffset(0, 5);
    dialog.setGraphicsEffect(shadow);

    // 创建主容器
    QWidget* mainContainer = new QWidget(&dialog);
    mainContainer->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #667eea, stop:1 #764ba2);
            border-radius: 15px;
        }
    )");

    // 创建布局
    QVBoxLayout* layout = new QVBoxLayout(mainContainer);
    layout->setContentsMargins(30, 30, 30, 30);
    layout->setSpacing(20);

    // 标题
    QLabel* titleLabel = new QLabel("请选择棋子颜色", mainContainer);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: #ffffff;
            font-size: 18px;
            font-weight: bold;
            background: transparent;
        }
    )");
    layout->addWidget(titleLabel);

    // 黑棋按钮
    QPushButton* blackButton = new QPushButton("执黑棋（先手）", mainContainer);
    blackButton->setMinimumHeight(45);
    blackButton->setStyleSheet(R"(
        QPushButton {
            background: rgba(30, 30, 30, 0.95);
            color: #ffffff;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(50, 50, 50, 0.95);
        }
        QPushButton:pressed {
            background: rgba(20, 20, 20, 0.95);
        }
    )");
    layout->addWidget(blackButton);

    // 白棋按钮
    QPushButton* whiteButton = new QPushButton("执白棋（后手）", mainContainer);
    whiteButton->setMinimumHeight(45);
    whiteButton->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 0.95);
            color: #2c3e50;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(240, 240, 240, 0.95);
        }
        QPushButton:pressed {
            background: rgba(220, 220, 220, 0.95);
        }
    )");
    layout->addWidget(whiteButton);

    dialog.setLayout(layout);

    // 连接按钮信号
    connect(blackButton, &QPushButton::clicked, &dialog, [&dialog, this]() {
        playerColor_ = 1;  // 黑棋
        dialog.accept();
    });

    connect(whiteButton, &QPushButton::clicked, &dialog, [&dialog, this]() {
        playerColor_ = 2;  // 白棋
        dialog.accept();
    });

    // 显示对话框
    int result = dialog.exec();

    // 如果用户关闭了对话框，默认选择黑棋
    if (result == QDialog::Rejected) {
        playerColor_ = 1;
    }

    startGame();
}

void OfflineGameDialog::startGame()
{
    gameController_->startGame();
    boardWidget_->reset();
    boardWidget_->setEnabled(true);

    // 黑棋先手，如果玩家选择白棋，AI先下
    isAITurn_ = (playerColor_ == 2);  // 玩家执白，AI先手
    updateGameInfo();

    // 如果AI先手，延迟让AI落子
    if (isAITurn_) {
        QTimer::singleShot(500, this, [this]() {
            makeAIMove();
        });
    }
}