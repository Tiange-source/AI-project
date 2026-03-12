#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QStyle>
#include "ui/offlinegamedialog.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
    , tcpClient_(nullptr)
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
        if (tcpClient_->isConnected()) {
            tcpClient_->disconnectFromServer();
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
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server";
    updateConnectionStatus();
}

void MainWindow::onConnectionError(const QString& error)
{
    qDebug() << "Connection error:" << error;
    updateConnectionStatus();
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
    tcpClient_->connectToServer("192.168.215.125", 8888);
}

void MainWindow::updateConnectionStatus()
{
    bool connected = tcpClient_->isConnected();
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