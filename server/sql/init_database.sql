-- 网络联机五子棋对战系统 - 数据库初始化脚本
-- 数据库: gomoku_db

-- 创建数据库
CREATE DATABASE IF NOT EXISTS gomoku_db DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

USE gomoku_db;

-- ========================================
-- 1. 用户表 (users)
-- ========================================
CREATE TABLE IF NOT EXISTS users (
    user_id INT PRIMARY KEY AUTO_INCREMENT COMMENT '用户唯一ID',
    username VARCHAR(50) UNIQUE NOT NULL COMMENT '用户登录名',
    password_hash VARCHAR(255) NOT NULL COMMENT '加密后的密码',
    email VARCHAR(100) DEFAULT NULL COMMENT '邮箱地址',
    nickname VARCHAR(50) DEFAULT NULL COMMENT '用户昵称',
    avatar_url VARCHAR(255) DEFAULT NULL COMMENT '头像URL',
    win_count INT DEFAULT 0 COMMENT '胜利场次',
    lose_count INT DEFAULT 0 COMMENT '失败场次',
    draw_count INT DEFAULT 0 COMMENT '平局场次',
    rating INT DEFAULT 1000 COMMENT '天梯积分',
    min_steps INT DEFAULT 0 COMMENT '最少步数获胜记录',
    total_games INT DEFAULT 0 COMMENT '总对局数',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    last_login TIMESTAMP DEFAULT NULL COMMENT '最后登录时间',
    INDEX idx_username (username),
    INDEX idx_rating (rating DESC),
    INDEX idx_total_games (total_games DESC)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='用户信息表';

-- ========================================
-- 2. 对局记录表 (game_records)
-- ========================================
CREATE TABLE IF NOT EXISTS game_records (
    game_id BIGINT PRIMARY KEY AUTO_INCREMENT COMMENT '对局唯一ID',
    room_id VARCHAR(20) DEFAULT NULL COMMENT '房间ID',
    player1_id INT NOT NULL COMMENT '玩家1用户ID',
    player2_id INT NOT NULL COMMENT '玩家2用户ID',
    winner_id INT DEFAULT NULL COMMENT '获胜者用户ID',
    game_mode ENUM('AI', 'ONLINE', 'MATCH') DEFAULT 'ONLINE' COMMENT '游戏模式',
    moves_data TEXT COMMENT '棋步序列(JSON格式)',
    total_moves INT DEFAULT 0 COMMENT '总步数',
    start_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '开始时间',
    end_time TIMESTAMP DEFAULT NULL COMMENT '结束时间',
    game_duration INT DEFAULT 0 COMMENT '对局时长(秒)',
    INDEX idx_player1 (player1_id),
    INDEX idx_player2 (player2_id),
    INDEX idx_winner (winner_id),
    INDEX idx_start_time (start_time DESC),
    INDEX idx_game_mode (game_mode),
    FOREIGN KEY (player1_id) REFERENCES users(user_id) ON DELETE CASCADE,
    FOREIGN KEY (player2_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='对局记录表';

-- ========================================
-- 3. 聊天记录表 (chat_history)
-- ========================================
CREATE TABLE IF NOT EXISTS chat_history (
    message_id BIGINT PRIMARY KEY AUTO_INCREMENT COMMENT '消息唯一ID',
    sender_id INT NOT NULL COMMENT '发送者用户ID',
    receiver_id INT DEFAULT NULL COMMENT '接收者用户ID(私聊)',
    room_id VARCHAR(20) DEFAULT NULL COMMENT '房间ID(房间聊天)',
    message_type ENUM('LOBBY', 'PRIVATE', 'ROOM') NOT NULL COMMENT '消息类型',
    content TEXT NOT NULL COMMENT '消息内容',
    send_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '发送时间',
    INDEX idx_sender (sender_id, send_time),
    INDEX idx_room (room_id, send_time),
    INDEX idx_type_time (message_type, send_time),
    FOREIGN KEY (sender_id) REFERENCES users(user_id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci COMMENT='聊天记录表';

-- ========================================
-- 4. 初始化管理员用户
-- ========================================
-- 密码: admin123 (使用bcrypt加密)
-- 注意: 实际部署时应该使用安全的密码哈希
INSERT INTO users (username, password_hash, email, nickname, avatar_url, win_count, lose_count, draw_count, rating, total_games)
VALUES ('admin', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', 'admin@gomoku.com', '系统管理员', '', 0, 0, 0, 1000, 0)
ON DUPLICATE KEY UPDATE username=username;

-- ========================================
-- 5. 创建视图：玩家统计信息
-- ========================================
CREATE OR REPLACE VIEW player_stats AS
SELECT 
    user_id,
    username,
    nickname,
    avatar_url,
    win_count,
    lose_count,
    draw_count,
    total_games,
    rating,
    CASE 
        WHEN total_games = 0 THEN 0.0
        ELSE ROUND((win_count * 100.0) / total_games, 2)
    END AS win_rate,
    CASE 
        WHEN total_games = 0 THEN 0
        ELSE ROUND((rating + (win_count - lose_count) * 10), 0)
    END AS calculated_rating,
    last_login
FROM users;

-- ========================================
-- 6. 创建视图：排行榜
-- ========================================
CREATE OR REPLACE VIEW leaderboard_win_count AS
SELECT 
    user_id,
    username,
    nickname,
    avatar_url,
    win_count,
    lose_count,
    draw_count,
    total_games,
    rating,
    CASE 
        WHEN total_games = 0 THEN 0.0
        ELSE ROUND((win_count * 100.0) / total_games, 2)
    END AS win_rate,
    RANK() OVER (ORDER BY win_count DESC, total_games ASC) AS rank
FROM users
WHERE total_games >= 3;

CREATE OR REPLACE VIEW leaderboard_rating AS
SELECT 
    user_id,
    username,
    nickname,
    avatar_url,
    win_count,
    lose_count,
    draw_count,
    total_games,
    rating,
    CASE 
        WHEN total_games = 0 THEN 0.0
        ELSE ROUND((win_count * 100.0) / total_games, 2)
    END AS win_rate,
    RANK() OVER (ORDER BY rating DESC) AS rank
FROM users
WHERE total_games >= 3;

CREATE OR REPLACE VIEW leaderboard_win_rate AS
SELECT 
    user_id,
    username,
    nickname,
    avatar_url,
    win_count,
    lose_count,
    draw_count,
    total_games,
    rating,
    CASE 
        WHEN total_games = 0 THEN 0.0
        ELSE ROUND((win_count * 100.0) / total_games, 2)
    END AS win_rate,
    RANK() OVER (ORDER BY 
        CASE WHEN total_games = 0 THEN 0 ELSE (win_count * 100.0) / total_games END DESC,
        total_games DESC
    ) AS rank
FROM users
WHERE total_games >= 3;

-- ========================================
-- 7. 创建存储过程：更新用户战绩
-- ========================================
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS update_user_stats(
    IN p_user_id INT,
    IN p_is_win BOOLEAN,
    IN p_steps INT
)
BEGIN
    IF p_is_win THEN
        UPDATE users 
        SET win_count = win_count + 1,
            total_games = total_games + 1,
            rating = rating + 25,
            min_steps = LEAST(min_steps, IF(min_steps = 0, p_steps, p_steps))
        WHERE user_id = p_user_id;
    ELSE
        UPDATE users 
        SET lose_count = lose_count + 1,
            total_games = total_games + 1,
            rating = GREATEST(0, rating - 10)
        WHERE user_id = p_user_id;
    END IF;
END //
DELIMITER ;

-- ========================================
-- 8. 创建存储过程：更新对局双方战绩
-- ========================================
DELIMITER //
CREATE PROCEDURE IF NOT EXISTS update_game_stats(
    IN p_player1_id INT,
    IN p_player2_id INT,
    IN p_winner_id INT,
    IN p_total_moves INT
)
BEGIN
    -- 更新玩家1战绩
    IF p_player1_id = p_winner_id THEN
        CALL update_user_stats(p_player1_id, TRUE, p_total_moves);
    ELSEIF p_winner_id IS NOT NULL THEN
        CALL update_user_stats(p_player1_id, FALSE, p_total_moves);
    ELSE
        -- 平局
        UPDATE users 
        SET draw_count = draw_count + 1,
            total_games = total_games + 1
        WHERE user_id = p_player1_id;
    END IF;
    
    -- 更新玩家2战绩
    IF p_player2_id = p_winner_id THEN
        CALL update_user_stats(p_player2_id, TRUE, p_total_moves);
    ELSEIF p_winner_id IS NOT NULL THEN
        CALL update_user_stats(p_player2_id, FALSE, p_total_moves);
    ELSE
        -- 平局
        UPDATE users 
        SET draw_count = draw_count + 1,
            total_games = total_games + 1
        WHERE user_id = p_player2_id;
    END IF;
END //
DELIMITER ;

-- 完成
SELECT '数据库初始化完成!' AS message;