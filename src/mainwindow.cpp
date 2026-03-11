#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
    , tcpClient_(nullptr)
    , dispatcher_(nullptr)
    , userProfile_(UserProfile::create())
{
    ui_->setupUi(this);

    // 初始化网络连接
    initNetwork();

    // 初始化UI
    initUI();

    // 注册消息处理器
    registerMessageHandlers();

    // 加载用户信息
    loadUserProfile();

    qDebug() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    if (tcpClient_) {
        tcpClient_->disconnectFromServer();
    }
    delete ui_;
}

void MainWindow::initNetwork()
{
    // 创建TCP客户端
    tcpClient_ = new TcpClient(this);

    // 创建消息分发器
    dispatcher_ = new MessageDispatcher(this);

    // 设置消息分发器
    tcpClient_->setMessageDispatcher(dispatcher_);

    // 连接信号
    connect(tcpClient_, &TcpClient::connected, this, &MainWindow::onConnected);
    connect(tcpClient_, &TcpClient::disconnected, this, &MainWindow::onDisconnected);
    connect(tcpClient_, &TcpClient::connectionError, this, &MainWindow::onConnectionError);

    qDebug() << "Network initialized";
}

void MainWindow::initUI()
{
    // 设置窗口标题
    setWindowTitle("网络联机五子棋客户端");

    // 设置窗口大小
    resize(1024, 768);

    // TODO: 初始化各个界面组件
    // - 登录界面
    // - 注册界面
    // - 游戏大厅
    // - 游戏房间
    // - 排行榜
    // - 观战界面

    qDebug() << "UI initialized";
}

void MainWindow::registerMessageHandlers()
{
    // TODO: 注册各种消息处理器
    // - 登录响应
    // - 注册响应
    // - 房间列表响应
    // - 游戏消息
    // - 聊天消息
    // - 排行榜响应
    // - 等等

    qDebug() << "Message handlers registered";
}

void MainWindow::loadUserProfile()
{
    if (userProfile_->loadFromLocal()) {
        qDebug() << "User profile loaded:" << userProfile_->getUsername();
        // TODO: 自动登录或显示欢迎界面
    } else {
        qDebug() << "No saved user profile";
        // TODO: 显示登录界面
    }
}

void MainWindow::onConnected()
{
    qDebug() << "Connected to server";
    QMessageBox::information(this, "连接成功", "已成功连接到服务器");
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server";
    QMessageBox::warning(this, "连接断开", "与服务器的连接已断开");
}

void MainWindow::onConnectionError(const QString& error)
{
    qDebug() << "Connection error:" << error;
    QMessageBox::critical(this, "连接错误", "连接服务器失败: " + error);
}