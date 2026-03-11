#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSharedPointer>
#include "network/tcpclient.h"
#include "network/messagedispatcher.h"
#include "data/userprofile.h"

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

    /**
     * @brief 初始化网络连接
     */
    void initNetwork();

    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 注册消息处理器
     */
    void registerMessageHandlers();

    /**
     * @brief 加载用户信息
     */
    void loadUserProfile();
};

#endif // MAINWINDOW_H