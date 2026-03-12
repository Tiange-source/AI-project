#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QStyle>
#include "ui/offlinegamedialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
    , windowsSocket_(nullptr)
    , dispatcher_(nullptr)
    , userProfile_(UserProfile::create())
    , titleBar_(nullptr)
    , gameController_(new GameController(this))
    , aiEngine_(new AIEngine(this))
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

    // 自动连接到服务器
    connectToServer();

    qDebug() << "MainWindow initialized";
}

MainWindow::~MainWindow()
{
    if (windowsSocket_) {
        windowsSocket_->disconnectFromServer();
        delete windowsSocket_;
        windowsSocket_ = nullptr;
    }
    delete ui_;
}

void MainWindow::initNetwork()
{
    // 创建Windows Socket客户端
    windowsSocket_ = new WindowsSocket(this);

    // 创建消息分发器
    dispatcher_ = new MessageDispatcher(this);

    // 设置消息分发器
    windowsSocket_->setMessageDispatcher(dispatcher_);

    // 连接信号
    connect(windowsSocket_, &WindowsSocket::connected, this, &MainWindow::onConnected);
    connect(windowsSocket_, &WindowsSocket::disconnected, this, &MainWindow::onDisconnected);
    connect(windowsSocket_, &WindowsSocket::connectionError, this, &MainWindow::onConnectionError);

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

    // 初始状态下隐藏登录按钮，等待连接成功后再显示
    ui_->loginButton->setVisible(false);
    ui_->loginButton->setEnabled(false);

    // 连接按钮信号
    connect(ui_->offlineButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Start game button clicked";
        showOfflineGame();
    });

    // 连接登录按钮信号
    connect(ui_->loginButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "Login button clicked";
        showLoginDialog();
    });

    // 连接菜单动作
    connect(ui_->actionConnect, &QAction::triggered, this, [this]() {
        qDebug() << "Connect to server";
        connectToServer();
    });

    connect(ui_->actionDisconnect, &QAction::triggered, this, [this]() {
        qDebug() << "Disconnect from server";
        if (windowsSocket_->isConnected()) {
            windowsSocket_->disconnectFromServer();
        }
    });

    connect(ui_->actionLogin, &QAction::triggered, ui_->offlineButton, &QPushButton::click);

    connect(ui_->actionExit, &QAction::triggered, this, &QMainWindow::close);

    connect(ui_->actionAbout, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于",
                          "<h3>网络联机五子棋客户端</h3>"
                          "<p>版本: 1.0.0</p>"
                          "<p>基于Qt 5.15.2和C++11开发</p>"
                          "<p>© 2024 Gomoku Project</p>");
    });

    // 检查是否有保存的用户信息
    if (userProfile_->isLoggedIn()) {
        ui_->welcomeLabel->setText("欢迎回来, " + userProfile_->getUsername() + "!");
    }

    // 更新连接状态
    updateConnectionStatus();

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
    updateConnectionStatus();

    // 发送测试登录请求以保持连接
    sendTestLoginRequest();
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server";
    updateConnectionStatus();
}

void MainWindow::onConnectionError(const QString& error)
{
    qDebug() << "Connection error:" << error;
    qWarning() << "Failed to connect to server 192.168.215.125:8888:" << error;
    updateConnectionStatus();

    // 显示连接失败的原因
    QMessageBox::warning(this, "连接失败",
                        "无法连接到服务器 192.168.215.125:8888\n\n"
                        "错误信息: " + error + "\n\n"
                        "可能的原因:\n"
                        "1. 服务器未启动\n"
                        "2. 网络不可达\n"
                        "3. 防火墙阻止连接\n"
                        "4. 服务器地址或端口错误\n\n"
                        "当前只能使用离线游戏功能。");
}

void MainWindow::showLoginDialog()
{
    qDebug() << "Show login dialog";
    // TODO: 实现登录对话框
    QMessageBox::information(this, "登录", "登录对话框即将实现");
}

void MainWindow::showOfflineGame()
{
    qDebug() << "Show offline game";
    OfflineGameDialog dialog(this);
    dialog.exec();
}

void MainWindow::connectToServer()
{
    qDebug() << "Connecting to server at 192.168.215.125:8888...";
    windowsSocket_->connectToServer("192.168.215.125", 8888);
}

void MainWindow::updateConnectionStatus()
{
    bool connected = windowsSocket_->isConnected();
    if (connected) {
        ui_->statusLabel->setText("已连接");
        ui_->statusLabel->setProperty("connected", true);
        // 连接成功时，显示登录和开始游戏按钮
        ui_->loginButton->setVisible(true);
        ui_->loginButton->setEnabled(true);
    } else {
        ui_->statusLabel->setText("未连接");
        ui_->statusLabel->setProperty("connected", false);
        // 连接失败时，只显示开始游戏按钮
        ui_->loginButton->setVisible(false);
        ui_->loginButton->setEnabled(false);
    }
    // 重新应用样式
    ui_->statusLabel->style()->unpolish(ui_->statusLabel);
    ui_->statusLabel->style()->polish(ui_->statusLabel);
}

void MainWindow::sendTestLoginRequest()
{
    qDebug() << "Sending test login request to keep connection alive";

    // 创建测试登录请求
    gomoku::LoginRequest loginRequest;
    loginRequest.set_username("test_user");
    loginRequest.set_password("test_password");

    // 发送消息
    if (windowsSocket_->sendMessage(loginRequest, gomoku::MessageType::LOGIN_REQUEST)) {
        qDebug() << "Test login request sent successfully";
    } else {
        qWarning() << "Failed to send test login request";
    }
}