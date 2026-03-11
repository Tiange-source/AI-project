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
    resize(1200, 800);

    // 加载样式表
    QFile styleFile(":/styles.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(styleFile.readAll());
        qApp->setStyleSheet(styleSheet);
        styleFile.close();
        qDebug() << "Style sheet loaded";
    } else {
        qWarning() << "Failed to load style sheet";
    }

    // 连接按钮信号
    connect(ui_->loginButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Login button clicked";
        // TODO: 显示登录对话框
        QMessageBox::information(this, "登录", "登录功能待实现");
    });

    connect(ui_->registerButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Register button clicked";
        // TODO: 显示注册对话框
        QMessageBox::information(this, "注册", "注册功能待实现");
    });

    // 连接菜单动作
    connect(ui_->actionConnect, &QAction::triggered, this, [this]() {
        qDebug() << "Connect to server";
        // TODO: 连接到服务器
        QMessageBox::information(this, "连接服务器", "连接功能待实现");
    });

    connect(ui_->actionDisconnect, &QAction::triggered, this, [this]() {
        qDebug() << "Disconnect from server";
        if (tcpClient_->isConnected()) {
            tcpClient_->disconnectFromServer();
        }
    });

    connect(ui_->actionLogin, &QAction::triggered, ui_->loginButton, &QPushButton::click);
    connect(ui_->actionRegister, &QAction::triggered, ui_->registerButton, &QPushButton::click);

    connect(ui_->actionExit, &QAction::triggered, this, &QMainWindow::close);

    connect(ui_->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于",
                          "<h3>网络联机五子棋客户端</h3>"
                          "<p>版本: 1.0.0</p>"
                          "<p>基于Qt 5.15.2和C++11开发</p>"
                          "<p>© 2024 Gomoku Project</p>");
    });

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