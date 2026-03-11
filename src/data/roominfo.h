#ifndef ROOMINFO_H
#define ROOMINFO_H

#include <QString>
#include <QJsonObject>
#include <QSharedPointer>

// 房间状态枚举
enum RoomState {
    WAITING = 0,   // 等待玩家
    GAMING = 1,    // 游戏中
    FINISHED = 2   // 已结束
};

// 用户信息简化结构
struct UserInfo {
    int userId;
    QString username;
    QString nickname;
    QString avatarUrl;
    int rating;

    UserInfo() : userId(0), rating(1200) {}
    UserInfo(int id, const QString& uname, const QString& nname, const QString& avatar, int r)
        : userId(id), username(uname), nickname(nname), avatarUrl(avatar), rating(r) {}

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);
};

/**
 * @brief 房间信息类
 *
 * 管理游戏房间的信息
 */
class RoomInfo
{
public:
    RoomInfo();
    ~RoomInfo();

    // 房间ID
    QString getRoomId() const;
    void setRoomId(const QString& roomId);

    // 房间名称
    QString getRoomName() const;
    void setRoomName(const QString& roomName);

    // 房主ID
    int getOwnerId() const;
    void setOwnerId(int ownerId);

    // 是否有密码
    bool hasPassword() const;
    void setHasPassword(bool hasPassword);

    // 房间状态
    RoomState getState() const;
    void setState(RoomState state);

    // 玩家1
    UserInfo getPlayer1() const;
    void setPlayer1(const UserInfo& player1);

    // 玩家2
    UserInfo getPlayer2() const;
    void setPlayer2(const UserInfo& player2);

    // 观战者数量
    int getSpectatorCount() const;
    void setSpectatorCount(int count);

    // 当前回合玩家ID
    int getCurrentTurn() const;
    void setCurrentTurn(int playerId);

    // 检查房间是否已满
    bool isFull() const;

    // 检查是否在游戏中
    bool isGaming() const;

    // 获取玩家数量
    int getPlayerCount() const;

    // 检查是否为房主
    bool isOwner(int userId) const;

    // 检查是否为玩家
    bool isPlayer(int userId) const;

    // 获取玩家信息
    UserInfo getPlayerInfo(int userId) const;

    // 转换为JSON对象
    QJsonObject toJson() const;

    // 从JSON对象加载
    void fromJson(const QJsonObject& json);

    // 创建共享指针
    static QSharedPointer<RoomInfo> create();

    // 获取房间状态字符串
    static QString getStateString(RoomState state);

private:
    QString roomId_;
    QString roomName_;
    int ownerId_;
    bool hasPassword_;
    RoomState state_;
    UserInfo player1_;
    UserInfo player2_;
    int spectatorCount_;
    int currentTurn_;
};

#endif // ROOMINFO_H