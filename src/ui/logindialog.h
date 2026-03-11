#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include "network/tcpclient.h"
#include "data/userprofile.h"

/**
 * @brief 登录对话框
 */
class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(TcpClient* tcpClient, QWidget* parent = nullptr);
    ~LoginDialog();

    /**
     * @brief 获取用户信息
     * @return 用户信息
     */
    QSharedPointer<UserProfile> getUserProfile() const;

signals:
    /**
     * @brief 登录成功信号
     * @param profile 用户信息
     */
    void loginSuccess(QSharedPointer<UserProfile> profile);

private slots:
    /**
     * @brief 登录按钮点击
     */
    void onLoginClicked();

    /**
     * @brief 注册按钮点击
     */
    void onRegisterClicked();

    /**
     * @brief 登录响应处理
     */
    void onLoginResponse(const QByteArray& data);

private:
    /**
     * @brief 初始化UI
     */
    void initUI();

    /**
     * @brief 连接信号
     */
    void connectSignals();

    /**
     * @brief 验证输入
     * @return 是否有效
     */
    bool validateInput();

private:
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* loginButton_;
    QPushButton* registerButton_;
    QPushButton* cancelButton_;
    QLabel* statusLabel_;

    TcpClient* tcpClient_;
    QSharedPointer<UserProfile> userProfile_;
};

#endif // LOGINDIALOG_H