#include "userprofile.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDebug>

// 配置文件键名
const QString UserProfile::KEY_USER_ID = "user_id";
const QString UserProfile::KEY_USERNAME = "username";
const QString UserProfile::KEY_NICKNAME = "nickname";
const QString UserProfile::KEY_AVATAR_URL = "avatar_url";
const QString UserProfile::KEY_WIN_COUNT = "win_count";
const QString UserProfile::KEY_LOSE_COUNT = "lose_count";
const QString UserProfile::KEY_DRAW_COUNT = "draw_count";
const QString UserProfile::KEY_RATING = "rating";
const QString UserProfile::KEY_TOKEN = "token";

UserProfile::UserProfile()
    : userId_(0)
    , winCount_(0)
    , loseCount_(0)
    , drawCount_(0)
    , rating_(1200)  // 默认积分
{
}

UserProfile::~UserProfile()
{
}

int UserProfile::getUserId() const
{
    return userId_;
}

void UserProfile::setUserId(int userId)
{
    userId_ = userId;
}

QString UserProfile::getUsername() const
{
    return username_;
}

void UserProfile::setUsername(const QString& username)
{
    username_ = username;
}

QString UserProfile::getNickname() const
{
    return nickname_;
}

void UserProfile::setNickname(const QString& nickname)
{
    nickname_ = nickname;
}

QString UserProfile::getAvatarUrl() const
{
    return avatarUrl_;
}

void UserProfile::setAvatarUrl(const QString& avatarUrl)
{
    avatarUrl_ = avatarUrl;
}

int UserProfile::getWinCount() const
{
    return winCount_;
}

void UserProfile::setWinCount(int winCount)
{
    winCount_ = winCount;
}

int UserProfile::getLoseCount() const
{
    return loseCount_;
}

void UserProfile::setLoseCount(int loseCount)
{
    loseCount_ = loseCount;
}

int UserProfile::getDrawCount() const
{
    return drawCount_;
}

void UserProfile::setDrawCount(int drawCount)
{
    drawCount_ = drawCount;
}

int UserProfile::getRating() const
{
    return rating_;
}

void UserProfile::setRating(int rating)
{
    rating_ = rating;
}

int UserProfile::getTotalGames() const
{
    return winCount_ + loseCount_ + drawCount_;
}

QString UserProfile::getToken() const
{
    return token_;
}

void UserProfile::setToken(const QString& token)
{
    token_ = token;
}

double UserProfile::getWinRate() const
{
    int total = getTotalGames();
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(winCount_) / total * 100.0;
}

bool UserProfile::isLoggedIn() const
{
    return !token_.isEmpty();
}

void UserProfile::clear()
{
    userId_ = 0;
    username_.clear();
    nickname_.clear();
    avatarUrl_.clear();
    winCount_ = 0;
    loseCount_ = 0;
    drawCount_ = 0;
    rating_ = 1200;
    token_.clear();
}

void UserProfile::saveToLocal()
{
    QSettings settings("GomokuClient", "User");

    settings.setValue(KEY_USER_ID, userId_);
    settings.setValue(KEY_USERNAME, username_);
    settings.setValue(KEY_NICKNAME, nickname_);
    settings.setValue(KEY_AVATAR_URL, avatarUrl_);
    settings.setValue(KEY_WIN_COUNT, winCount_);
    settings.setValue(KEY_LOSE_COUNT, loseCount_);
    settings.setValue(KEY_DRAW_COUNT, drawCount_);
    settings.setValue(KEY_RATING, rating_);
    settings.setValue(KEY_TOKEN, token_);

    settings.sync();

    qDebug() << "User profile saved to local";
}

bool UserProfile::loadFromLocal()
{
    QSettings settings("GomokuClient", "User");

    userId_ = settings.value(KEY_USER_ID, 0).toInt();
    username_ = settings.value(KEY_USERNAME, "").toString();
    nickname_ = settings.value(KEY_NICKNAME, "").toString();
    avatarUrl_ = settings.value(KEY_AVATAR_URL, "").toString();
    winCount_ = settings.value(KEY_WIN_COUNT, 0).toInt();
    loseCount_ = settings.value(KEY_LOSE_COUNT, 0).toInt();
    drawCount_ = settings.value(KEY_DRAW_COUNT, 0).toInt();
    rating_ = settings.value(KEY_RATING, 1200).toInt();
    token_ = settings.value(KEY_TOKEN, "").toString();

    bool hasToken = !token_.isEmpty();
    if (hasToken) {
        qDebug() << "User profile loaded from local:" << username_;
    }

    return hasToken;
}

QJsonObject UserProfile::toJson() const
{
    QJsonObject json;
    json[KEY_USER_ID] = userId_;
    json[KEY_USERNAME] = username_;
    json[KEY_NICKNAME] = nickname_;
    json[KEY_AVATAR_URL] = avatarUrl_;
    json[KEY_WIN_COUNT] = winCount_;
    json[KEY_LOSE_COUNT] = loseCount_;
    json[KEY_DRAW_COUNT] = drawCount_;
    json[KEY_RATING] = rating_;
    json["total_games"] = getTotalGames();
    json["win_rate"] = getWinRate();
    return json;
}

void UserProfile::fromJson(const QJsonObject& json)
{
    userId_ = json[KEY_USER_ID].toInt(0);
    username_ = json[KEY_USERNAME].toString();
    nickname_ = json[KEY_NICKNAME].toString();
    avatarUrl_ = json[KEY_AVATAR_URL].toString();
    winCount_ = json[KEY_WIN_COUNT].toInt(0);
    loseCount_ = json[KEY_LOSE_COUNT].toInt(0);
    drawCount_ = json[KEY_DRAW_COUNT].toInt(0);
    rating_ = json[KEY_RATING].toInt(1200);
}

QSharedPointer<UserProfile> UserProfile::create()
{
    return QSharedPointer<UserProfile>::create();
}