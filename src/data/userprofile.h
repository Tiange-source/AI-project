#ifndef USERPROFILE_H
#define USERPROFILE_H

#include <QString>
#include <QJsonObject>
#include <QSettings>
#include <QSharedPointer>

/**
 * @brief 用户信息类
 *
 * 管理用户信息和本地Token保存
 */
class UserProfile
{
public:
    UserProfile();
    ~UserProfile();

    // 用户ID
    int getUserId() const;
    void setUserId(int userId);

    // 用户名
    QString getUsername() const;
    void setUsername(const QString& username);

    // 昵称
    QString getNickname() const;
    void setNickname(const QString& nickname);

    // 头像URL
    QString getAvatarUrl() const;
    void setAvatarUrl(const QString& avatarUrl);

    // 胜场数
    int getWinCount() const;
    void setWinCount(int winCount);

    // 负场数
    int getLoseCount() const;
    void setLoseCount(int loseCount);

    // 平局数
    int getDrawCount() const;
    void setDrawCount(int drawCount);

    // 积分
    int getRating() const;
    void setRating(int rating);

    // 总场次
    int getTotalGames() const;

    // Token
    QString getToken() const;
    void setToken(const QString& token);

    // 计算胜率
    double getWinRate() const;

    // 检查是否已登录（有Token）
    bool isLoggedIn() const;

    // 清除用户信息（退出登录）
    void clear();

    // 保存到本地配置文件
    void saveToLocal();

    // 从本地配置文件加载
    bool loadFromLocal();

    // 转换为JSON对象
    QJsonObject toJson() const;

    // 从JSON对象加载
    void fromJson(const QJsonObject& json);

    // 创建共享指针
    static QSharedPointer<UserProfile> create();

private:
    int userId_;
    QString username_;
    QString nickname_;
    QString avatarUrl_;
    int winCount_;
    int loseCount_;
    int drawCount_;
    int rating_;
    QString token_;

    // 配置文件键名
    static const QString KEY_USER_ID;
    static const QString KEY_USERNAME;
    static const QString KEY_NICKNAME;
    static const QString KEY_AVATAR_URL;
    static const QString KEY_WIN_COUNT;
    static const QString KEY_LOSE_COUNT;
    static const QString KEY_DRAW_COUNT;
    static const QString KEY_RATING;
    static const QString KEY_TOKEN;
};

#endif // USERPROFILE_H