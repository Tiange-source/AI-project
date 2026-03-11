#ifndef MYSQL_CLIENT_H
#define MYSQL_CLIENT_H

#include <mysql/mysql.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace gomoku {

// 数据库配置
struct MySQLConfig {
    std::string host = "localhost";
    int port = 3306;
    std::string database = "gomoku_db";
    std::string username = "root";
    std::string password = "";
    int connectionPoolSize = 10;
};

// 数据库查询结果
class MySQLResult {
public:
    MySQLResult(MYSQL_RES* res);
    ~MySQLResult();

    bool next();
    int getInt(int col);
    int getInt(const std::string& colName);
    std::string getString(int col);
    std::string getString(const std::string& colName);
    long long getLongLong(int col);
    long long getLongLong(const std::string& colName);
    double getDouble(int col);
    double getDouble(const std::string& colName);
    
    int getRowCount() const;
    int getFieldCount() const;
    std::string getFieldName(int col) const;

private:
    MYSQL_RES* result_;
    MYSQL_ROW row_;
    unsigned long* lengths_;
};

// MySQL连接类
class MySQLConnection {
public:
    MySQLConnection();
    ~MySQLConnection();

    bool connect(const MySQLConfig& config);
    void close();
    bool isConnected() const;
    
    // 执行查询，返回结果
    std::shared_ptr<MySQLResult> query(const std::string& sql);
    
    // 执行更新（INSERT/UPDATE/DELETE），返回影响的行数
    int execute(const std::string& sql);
    
    // 获取最后插入的ID
    int getLastInsertId();
    
    // 开始事务
    bool beginTransaction();
    
    // 提交事务
    bool commit();
    
    // 回滚事务
    bool rollback();
    
    // 获取错误信息
    std::string getError() const;

private:
    MYSQL* mysql_;
    bool connected_;
    MySQLConfig config_;
};

// MySQL客户端类（连接池）
class MySQLClient {
public:
    using ConnectionPtr = std::shared_ptr<MySQLConnection>;
    
    MySQLClient();
    ~MySQLClient();

    // 初始化客户端
    bool initialize(const MySQLConfig& config);
    
    // 关闭客户端
    void close();
    
    // 获取一个连接
    ConnectionPtr getConnection();
    
    // 释放连接（放回连接池）
    void releaseConnection(ConnectionPtr conn);
    
    // 执行查询（自动获取和释放连接）
    std::shared_ptr<MySQLResult> query(const std::string& sql);
    
    // 执行更新（自动获取和释放连接）
    int execute(const std::string& sql);
    
    // 执行事务
    bool transaction(std::function<bool(MySQLConnection* conn)> callback);

private:
    MySQLConfig config_;
    std::vector<ConnectionPtr> pool_;
    std::mutex mutex_;
    int poolSize_;
};

} // namespace gomoku

#endif // MYSQL_CLIENT_H