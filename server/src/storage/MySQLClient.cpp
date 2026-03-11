#include "storage/MySQLClient.h"
#include <iostream>
#include <sstream>

namespace gomoku {

// ========================================
// MySQLResult 实现
// ========================================
MySQLResult::MySQLResult(MYSQL_RES* res) 
    : result_(res), row_(nullptr), lengths_(nullptr) {
}

MySQLResult::~MySQLResult() {
    if (result_) {
        mysql_free_result(result_);
    }
}

bool MySQLResult::next() {
    if (!result_) {
        return false;
    }
    row_ = mysql_fetch_row(result_);
    if (row_) {
        lengths_ = mysql_fetch_lengths(result_);
        return true;
    }
    return false;
}

int MySQLResult::getInt(int col) {
    if (!row_ || col >= mysql_num_fields(result_)) {
        return 0;
    }
    return row_[col] ? std::stoi(row_[col]) : 0;
}

int MySQLResult::getInt(const std::string& colName) {
    unsigned int numFields = mysql_num_fields(result_);
    MYSQL_FIELD* fields = mysql_fetch_fields(result_);
    
    for (unsigned int i = 0; i < numFields; ++i) {
        if (fields[i].name == colName) {
            return getInt(i);
        }
    }
    return 0;
}

std::string MySQLResult::getString(int col) {
    if (!row_ || col >= mysql_num_fields(result_)) {
        return "";
    }
    return row_[col] ? std::string(row_[col], lengths_[col]) : "";
}

std::string MySQLResult::getString(const std::string& colName) {
    unsigned int numFields = mysql_num_fields(result_);
    MYSQL_FIELD* fields = mysql_fetch_fields(result_);
    
    for (unsigned int i = 0; i < numFields; ++i) {
        if (fields[i].name == colName) {
            return getString(i);
        }
    }
    return "";
}

long long MySQLResult::getLongLong(int col) {
    if (!row_ || col >= mysql_num_fields(result_)) {
        return 0;
    }
    return row_[col] ? std::stoll(row_[col]) : 0;
}

long long MySQLResult::getLongLong(const std::string& colName) {
    unsigned int numFields = mysql_num_fields(result_);
    MYSQL_FIELD* fields = mysql_fetch_fields(result_);
    
    for (unsigned int i = 0; i < numFields; ++i) {
        if (fields[i].name == colName) {
            return getLongLong(i);
        }
    }
    return 0;
}

double MySQLResult::getDouble(int col) {
    if (!row_ || col >= mysql_num_fields(result_)) {
        return 0.0;
    }
    return row_[col] ? std::stod(row_[col]) : 0.0;
}

double MySQLResult::getDouble(const std::string& colName) {
    unsigned int numFields = mysql_num_fields(result_);
    MYSQL_FIELD* fields = mysql_fetch_fields(result_);
    
    for (unsigned int i = 0; i < numFields; ++i) {
        if (fields[i].name == colName) {
            return getDouble(i);
        }
    }
    return 0.0;
}

int MySQLResult::getRowCount() const {
    if (!result_) {
        return 0;
    }
    return mysql_num_rows(result_);
}

int MySQLResult::getFieldCount() const {
    if (!result_) {
        return 0;
    }
    return mysql_num_fields(result_);
}

std::string MySQLResult::getFieldName(int col) const {
    if (!result_) {
        return "";
    }
    MYSQL_FIELD* fields = mysql_fetch_fields(result_);
    if (col < mysql_num_fields(result_)) {
        return fields[col].name;
    }
    return "";
}

// ========================================
// MySQLConnection 实现
// ========================================
MySQLConnection::MySQLConnection() 
    : mysql_(nullptr), connected_(false) {
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        std::cerr << "MySQL init failed" << std::endl;
    }
}

MySQLConnection::~MySQLConnection() {
    close();
}

bool MySQLConnection::connect(const MySQLConfig& config) {
    if (!mysql_) {
        return false;
    }
    
    config_ = config;
    
    MYSQL* result = mysql_real_connect(
        mysql_,
        config.host.c_str(),
        config.username.c_str(),
        config.password.c_str(),
        config.database.c_str(),
        config.port,
        nullptr,
        0
    );
    
    if (result) {
        connected_ = true;
        // 设置字符集为UTF8
        mysql_set_character_set(mysql_, "utf8mb4");
        return true;
    }
    
    std::cerr << "MySQL connect failed: " << mysql_error(mysql_) << std::endl;
    return false;
}

void MySQLConnection::close() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        connected_ = false;
    }
}

bool MySQLConnection::isConnected() const {
    return connected_ && mysql_ != nullptr;
}

std::shared_ptr<MySQLResult> MySQLConnection::query(const std::string& sql) {
    if (!isConnected()) {
        return nullptr;
    }
    
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        std::cerr << "MySQL query failed: " << mysql_error(mysql_) << std::endl;
        std::cerr << "SQL: " << sql << std::endl;
        return nullptr;
    }
    
    MYSQL_RES* result = mysql_store_result(mysql_);
    if (!result) {
        if (mysql_field_count(mysql_) == 0) {
            // 没有结果集（INSERT/UPDATE/DELETE）
            return std::make_shared<MySQLResult>(nullptr);
        }
        std::cerr << "MySQL store result failed: " << mysql_error(mysql_) << std::endl;
        return nullptr;
    }
    
    return std::make_shared<MySQLResult>(result);
}

int MySQLConnection::execute(const std::string& sql) {
    if (!isConnected()) {
        return -1;
    }
    
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        std::cerr << "MySQL execute failed: " << mysql_error(mysql_) << std::endl;
        std::cerr << "SQL: " << sql << std::endl;
        return -1;
    }
    
    return static_cast<int>(mysql_affected_rows(mysql_));
}

int MySQLConnection::getLastInsertId() {
    if (!isConnected()) {
        return -1;
    }
    return static_cast<int>(mysql_insert_id(mysql_));
}

bool MySQLConnection::beginTransaction() {
    if (!isConnected()) {
        return false;
    }
    return mysql_autocommit(mysql_, 0) == 0;
}

bool MySQLConnection::commit() {
    if (!isConnected()) {
        return false;
    }
    bool result = mysql_commit(mysql_) == 0;
    mysql_autocommit(mysql_, 1);
    return result;
}

bool MySQLConnection::rollback() {
    if (!isConnected()) {
        return false;
    }
    bool result = mysql_rollback(mysql_) == 0;
    mysql_autocommit(mysql_, 1);
    return result;
}

std::string MySQLConnection::getError() const {
    if (mysql_) {
        return mysql_error(mysql_);
    }
    return "";
}

// ========================================
// MySQLClient 实现
// ========================================
MySQLClient::MySQLClient() : poolSize_(0) {
}

MySQLClient::~MySQLClient() {
    close();
}

bool MySQLClient::initialize(const MySQLConfig& config) {
    config_ = config;
    poolSize_ = config.connectionPoolSize;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 创建连接池
    for (int i = 0; i < poolSize_; ++i) {
        auto conn = std::make_shared<MySQLConnection>();
        if (conn->connect(config)) {
            pool_.push_back(conn);
        } else {
            std::cerr << "Failed to create MySQL connection " << i + 1 << std::endl;
            return false;
        }
    }
    
    std::cout << "MySQL connection pool initialized with " << pool_.size() << " connections" << std::endl;
    return true;
}

void MySQLClient::close() {
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.clear();
}

MySQLClient::ConnectionPtr MySQLClient::getConnection() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 简单实现：直接返回第一个可用连接
    // 实际项目中应该使用更复杂的连接池管理
    if (pool_.empty()) {
        // 连接池为空，创建新连接
        auto conn = std::make_shared<MySQLConnection>();
        if (conn->connect(config_)) {
            return conn;
        }
        return nullptr;
    }
    
    auto conn = pool_.front();
    pool_.erase(pool_.begin());
    return conn;
}

void MySQLClient::releaseConnection(ConnectionPtr conn) {
    if (!conn) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    pool_.push_back(conn);
}

std::shared_ptr<MySQLResult> MySQLClient::query(const std::string& sql) {
    auto conn = getConnection();
    if (!conn) {
        return nullptr;
    }
    
    auto result = conn->query(sql);
    releaseConnection(conn);
    return result;
}

int MySQLClient::execute(const std::string& sql) {
    auto conn = getConnection();
    if (!conn) {
        return -1;
    }
    
    int affectedRows = conn->execute(sql);
    releaseConnection(conn);
    return affectedRows;
}

bool MySQLClient::transaction(std::function<bool(MySQLConnection* conn)> callback) {
    auto conn = getConnection();
    if (!conn) {
        return false;
    }
    
    if (!conn->beginTransaction()) {
        releaseConnection(conn);
        return false;
    }
    
    bool success = callback(conn.get());
    
    if (success) {
        conn->commit();
    } else {
        conn->rollback();
    }
    
    releaseConnection(conn);
    return success;
}

} // namespace gomoku