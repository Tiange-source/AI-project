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
    , titleBar_(nullptr)
{
    // 设置无边框窗口
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);

    ui_->setupUi(this);

    // 初始化网络连接
    initNetwork();

    // 初始化UI
    initUI();

    // 初始化标题栏
    initTitleBar();

    // 设置窗口阴影
    setWindowShadow();

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

    // 保存中央窗口部件的指针，因为后续会重新设置
    QWidget* centralWidget = ui_->centralwidget;

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

void MainWindow::initTitleBar()
{
    // 创建自定义标题栏
    titleBar_ = new TitleBar(this);

    // 连接信号
    connect(titleBar_, &TitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(titleBar_, &TitleBar::maximizeClicked, this, [this](bool maximized) {
        if (maximized) {
            showMaximized();
        } else {
            showNormal();
        }
    });
    connect(titleBar_, &TitleBar::closeClicked, this, &QMainWindow::close);

    qDebug() << "Title bar initialized";
}

void MainWindow::setWindowShadow()
{
    // 设置窗口背景透明
    setAttribute(Qt::WA_TranslucentBackground, true);

    // 创建阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setOffset(0, 5);

    // 创建主容器
    QWidget* mainContainer = new QWidget(this);
    mainContainer->setGraphicsEffect(shadow);
    mainContainer->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #667eea, stop:1 #764ba2);
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 0.3);
        }
    )");

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 添加标题栏
    if (titleBar_) {
        mainLayout->addWidget(titleBar_);
    }

    // 添加中央窗口部件
    if (ui_->centralwidget) {
        ui_->centralwidget->setParent(mainContainer);
        mainLayout->addWidget(ui_->centralwidget);
    }

    // 设置为主窗口的中央部件
    QMainWindow::setCentralWidget(mainContainer);

    qDebug() << "Window shadow set";
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