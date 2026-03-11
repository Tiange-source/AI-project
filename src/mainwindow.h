#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>
#include <QFile>
#include <QGraphicsDropShadowEffect>
#include "network/tcpclient.h"
#include "network/messagedispatcher.h"
#include "data/userprofile.h"
#include "logic/gamecontroller.h"
#include "logic/aiengine.h"
#include "ui/titlebar.h"
#include "ui/offlinegamedialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 主窗口类
 *
 * 应用程序的主窗口，管理所有界面和组件
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * @brief 连接成功槽函数
     */
    void onConnected();

    /**
     * @brief 连接断开槽函数
     */
    void onDisconnected();

    /**
     * @brief 连接错误槽函数
     * @param error 错误信息
     */
    void onConnectionError(const QString& error);

private:
    Ui::MainWindow* ui_;
    TcpClient* tcpClient_;
    MessageDispatcher* dispatcher_;
    QSharedPointer<UserProfile> userProfile_;
    TitleBar* titleBar_;
    GameController* gameController_;
    AIEngine* aiEngine_;

    /**
     * @brief 初始化网络连接
     */
    void initNetwork();

    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 初始化标题栏
     */
    void initTitleBar();

    /**
     * @brief 设置窗口阴影
     */
    void setWindowShadow();

    /**
     * @brief 注册消息处理器
     */
    void registerMessageHandlers();

    /**
     * @brief 加载用户信息
     */
    void loadUserProfile();

    /**
     * @brief 显示登录对话框
     */
    void showLoginDialog();

    /**
     * @brief 显示离线对战
     */
    void showOfflineGame();

    /**
     * @brief 连接到服务器
     */
    void connectToServer();

    /**
     * @brief 更新连接状态UI
     */
    void updateConnectionStatus();
};

#endif // MAINWINDOW_H