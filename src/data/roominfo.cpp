#include "roominfo.h"

RoomInfo::RoomInfo()
    : ownerId_(0)
    , hasPassword_(false)
    , state_(WAITING)
    , spectatorCount_(0)
    , currentTurn_(0)
{
}

RoomInfo::~RoomInfo()
{
}

QString RoomInfo::getRoomId() const
{
    return roomId_;
}

void RoomInfo::setRoomId(const QString& roomId)
{
    roomId_ = roomId;
}

QString RoomInfo::getRoomName() const
{
    return roomName_;
}

void RoomInfo::setRoomName(const QString& roomName)
{
    roomName_ = roomName;
}

int RoomInfo::getOwnerId() const
{
    return ownerId_;
}

void RoomInfo::setOwnerId(int ownerId)
{
    ownerId_ = ownerId;
}

bool RoomInfo::hasPassword() const
{
    return hasPassword_;
}

void RoomInfo::setHasPassword(bool hasPassword)
{
    hasPassword_ = hasPassword;
}

RoomState RoomInfo::getState() const
{
    return state_;
}

void RoomInfo::setState(RoomState state)
{
    state_ = state;
}

UserInfo RoomInfo::getPlayer1() const
{
    return player1_;
}

void RoomInfo::setPlayer1(const UserInfo& player1)
{
    player1_ = player1;
}

UserInfo RoomInfo::getPlayer2() const
{
    return player2_;
}

void RoomInfo::setPlayer2(const UserInfo& player2)
{
    player2_ = player2;
}

int RoomInfo::getSpectatorCount() const
{
    return spectatorCount_;
}

void RoomInfo::setSpectatorCount(int count)
{
    spectatorCount_ = count;
}

int RoomInfo::getCurrentTurn() const
{
    return currentTurn_;
}

void RoomInfo::setCurrentTurn(int playerId)
{
    currentTurn_ = playerId;
}

bool RoomInfo::isFull() const
{
    return player1_.userId != 0 && player2_.userId != 0;
}

bool RoomInfo::isGaming() const
{
    return state_ == GAMING;
}

int RoomInfo::getPlayerCount() const
{
    int count = 0;
    if (player1_.userId != 0) count++;
    if (player2_.userId != 0) count++;
    return count;
}

bool RoomInfo::isOwner(int userId) const
{
    return ownerId_ == userId;
}

bool RoomInfo::isPlayer(int userId) const
{
    return player1_.userId == userId || player2_.userId == userId;
}

UserInfo RoomInfo::getPlayerInfo(int userId) const
{
    if (player1_.userId == userId) {
        return player1_;
    } else if (player2_.userId == userId) {
        return player2_;
    }
    return UserInfo();
}

QJsonObject RoomInfo::toJson() const
{
    QJsonObject json;
    json["room_id"] = roomId_;
    json["room_name"] = roomName_;
    json["owner_id"] = ownerId_;
    json["has_password"] = hasPassword_;
    json["state"] = static_cast<int>(state_);
    json["player1"] = player1_.toJson();
    json["player2"] = player2_.toJson();
    json["spectator_count"] = spectatorCount_;
    json["current_turn"] = currentTurn_;
    json["player_count"] = getPlayerCount();
    json["is_full"] = isFull();
    json["is_gaming"] = isGaming();
    return json;
}

void RoomInfo::fromJson(const QJsonObject& json)
{
    roomId_ = json["room_id"].toString();
    roomName_ = json["room_name"].toString();
    ownerId_ = json["owner_id"].toInt(0);
    hasPassword_ = json["has_password"].toBool(false);
    state_ = static_cast<RoomState>(json["state"].toInt(0));
    player1_.fromJson(json["player1"].toObject());
    player2_.fromJson(json["player2"].toObject());
    spectatorCount_ = json["spectator_count"].toInt(0);
    currentTurn_ = json["current_turn"].toInt(0);
}

QSharedPointer<RoomInfo> RoomInfo::create()
{
    return QSharedPointer<RoomInfo>::create();
}

QString RoomInfo::getStateString(RoomState state)
{
    switch (state) {
        case WAITING:
            return "等待中";
        case GAMING:
            return "游戏中";
        case FINISHED:
            return "已结束";
        default:
            return "未知";
    }
}

// UserInfo方法实现
QJsonObject UserInfo::toJson() const
{
    QJsonObject json;
    json["user_id"] = userId;
    json["username"] = username;
    json["nickname"] = nickname;
    json["avatar_url"] = avatarUrl;
    json["rating"] = rating;
    return json;
}

void UserInfo::fromJson(const QJsonObject& json)
{
    userId = json["user_id"].toInt(0);
    username = json["username"].toString();
    nickname = json["nickname"].toString();
    avatarUrl = json["avatar_url"].toString();
    rating = json["rating"].toInt(1200);
}