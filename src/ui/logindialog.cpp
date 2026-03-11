#include "logindialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>

LoginDialog::LoginDialog(TcpClient* tcpClient, QWidget* parent)
    : QDialog(parent)
    , usernameEdit_(nullptr)
    , passwordEdit_(nullptr)
    , loginButton_(nullptr)
    , registerButton_(nullptr)
    , cancelButton_(nullptr)
    , statusLabel_(nullptr)
    , tcpClient_(tcpClient)
    , userProfile_(UserProfile::create())
{
    initUI();
    connectSignals();

    // 设置无边框对话框
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);
}

LoginDialog::~LoginDialog()
{
}

QSharedPointer<UserProfile> LoginDialog::getUserProfile() const
{
    return userProfile_;
}

void LoginDialog::initUI()
{
    // 设置对话框大小
    setFixedSize(450, 500);

    // 创建主容器
    QWidget* mainContainer = new QWidget(this);
    mainContainer->setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #667eea, stop:1 #764ba2);
            border-radius: 20px;
        }
    )");

    // 设置阴影效果
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 150));
    shadow->setOffset(0, 5);
    setGraphicsEffect(shadow);

    // 创建布局
    QVBoxLayout* mainLayout = new QVBoxLayout(mainContainer);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(20);

    // 标题
    QLabel* titleLabel = new QLabel("用户登录", mainContainer);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(R"(
        QLabel {
            color: #ffffff;
            font-size: 28px;
            font-weight: bold;
            background: transparent;
        }
    )");
    mainLayout->addWidget(titleLabel);

    // 分隔线
    QLabel* separator = new QLabel(mainContainer);
    separator->setFixedHeight(2);
    separator->setStyleSheet(R"(
        QLabel {
            background: rgba(255, 255, 255, 0.3);
            border-radius: 1px;
        }
    )");
    mainLayout->addWidget(separator);

    // 表单布局
    QFormLayout* formLayout = new QFormLayout();
    formLayout->setSpacing(20);
    formLayout->setContentsMargins(20, 0, 20, 0);

    // 用户名输入
    usernameEdit_ = new QLineEdit(mainContainer);
    usernameEdit_->setPlaceholderText("请输入用户名");
    usernameEdit_->setStyleSheet(R"(
        QLineEdit {
            background: rgba(255, 255, 255, 0.95);
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            padding: 12px 15px;
            font-size: 14px;
            color: #2c3e50;
        }
        QLineEdit:focus {
            border-color: #ffffff;
            background: #ffffff;
        }
    )");
    formLayout->addRow("用户名:", usernameEdit_);

    // 密码输入
    passwordEdit_ = new QLineEdit(mainContainer);
    passwordEdit_->setPlaceholderText("请输入密码");
    passwordEdit_->setEchoMode(QLineEdit::Password);
    passwordEdit_->setStyleSheet(R"(
        QLineEdit {
            background: rgba(255, 255, 255, 0.95);
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            padding: 12px 15px;
            font-size: 14px;
            color: #2c3e50;
        }
        QLineEdit:focus {
            border-color: #ffffff;
            background: #ffffff;
        }
    )");
    formLayout->addRow("密码:", passwordEdit_);

    mainLayout->addLayout(formLayout);

    // 状态标签
    statusLabel_ = new QLabel(mainContainer);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setWordWrap(true);
    statusLabel_->setStyleSheet(R"(
        QLabel {
            color: #ffffff;
            background: transparent;
            font-size: 12px;
        }
    )");
    mainLayout->addWidget(statusLabel_);

    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(15);

    // 登录按钮
    loginButton_ = new QPushButton("登录", mainContainer);
    loginButton_->setMinimumHeight(45);
    loginButton_->setStyleSheet(R"(
        QPushButton {
            background: #ffffff;
            color: #667eea;
            border: none;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.9);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.8);
        }
    )");
    buttonLayout->addWidget(loginButton_);

    // 注册按钮
    registerButton_ = new QPushButton("注册账号", mainContainer);
    registerButton_->setMinimumHeight(45);
    registerButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #ffffff;
            border: 2px solid #ffffff;
            border-radius: 10px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.2);
        }
        QPushButton:pressed {
            background: rgba(255, 255, 255, 0.3);
        }
    )");
    buttonLayout->addWidget(registerButton_);

    mainLayout->addLayout(buttonLayout);

    // 取消按钮
    cancelButton_ = new QPushButton("取消", mainContainer);
    cancelButton_->setMinimumHeight(40);
    cancelButton_->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: rgba(255, 255, 255, 0.7);
            border: none;
            border-radius: 10px;
            font-size: 14px;
        }
        QPushButton:hover {
            color: #ffffff;
            background: rgba(255, 255, 255, 0.1);
        }
    )");
    mainLayout->addWidget(cancelButton_);

    // 设置主布局
    QVBoxLayout* dialogLayout = new QVBoxLayout(this);
    dialogLayout->setContentsMargins(0, 0, 0, 0);
    dialogLayout->addWidget(mainContainer);

    // 设置主布局
    setLayout(dialogLayout);
}

void LoginDialog::connectSignals()
{
    connect(loginButton_, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton_, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &QDialog::reject);
}

void LoginDialog::onLoginClicked()
{
    if (!validateInput()) {
        return;
    }

    QString username = usernameEdit_->text();
    QString password = passwordEdit_->text();

    statusLabel_->setText("正在登录...");

    // TODO: 发送登录请求到服务器
    // 暂时模拟登录成功
    QMessageBox::information(this, "登录成功", "登录成功！");

    userProfile_->setUsername(username);
    userProfile_->setToken("simulated_token_" + username);
    userProfile_->saveToLocal();

    emit loginSuccess(userProfile_);
    accept();
}

void LoginDialog::onRegisterClicked()
{
    // TODO: 显示注册对话框
    QMessageBox::information(this, "注册", "注册功能待实现");
}

void LoginDialog::onLoginResponse(const QByteArray& data)
{
    // TODO: 处理登录响应
}

bool LoginDialog::validateInput()
{
    QString username = usernameEdit_->text().trimmed();
    QString password = passwordEdit_->text();

    if (username.isEmpty()) {
        statusLabel_->setText("请输入用户名");
        usernameEdit_->setFocus();
        return false;
    }

    if (username.length() < 3 || username.length() > 20) {
        statusLabel_->setText("用户名长度应在3-20字符之间");
        usernameEdit_->setFocus();
        return false;
    }

    if (password.isEmpty()) {
        statusLabel_->setText("请输入密码");
        passwordEdit_->setFocus();
        return false;
    }

    if (password.length() < 6) {
        statusLabel_->setText("密码长度不能少于6位");
        passwordEdit_->setFocus();
        return false;
    }

    statusLabel_->clear();
    return true;
}