#ifndef OFFLINEGAMEDIALOG_H
#define OFFLINEGAMEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QSharedPointer>
#include "ui/boardwidget.h"
#include "logic/gamecontroller.h"
#include "logic/aiengine.h"
#include "data/userprofile.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OfflineGameDialog; }
QT_END_NAMESPACE

/**
 * @brief 离线对战对话框
 *
 * 提供本地单机对战功能，玩家可以与AI进行对战
 */
class OfflineGameDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OfflineGameDialog(QWidget* parent = nullptr);
    ~OfflineGameDialog();

private slots:
    void onPieceClicked(int row, int col);
    void onRestartButtonClicked();
    void onBackButtonClicked();
    void onUndoButtonClicked();

private:
    Ui::OfflineGameDialog* ui_;
    BoardWidget* boardWidget_;
    GameController* gameController_;
    AIEngine* aiEngine_;
    QSharedPointer<UserProfile> userProfile_;
    QLabel* turnLabel_;
    QPushButton* undoButton_;
    QPushButton* restartButton_;
    QPushButton* backButton_;

    int playerColor_;  // 玩家颜色：1=黑棋，2=白棋
    bool isAITurn_;    // 是否AI回合

    void initUI();
    void connectSignals();
    void makeAIMove();
    void updateGameInfo();
    void endGame(int winner);
    void showColorSelectionDialog();
    void startGame();
};

#endif // OFFLINEGAMEDIALOG_H